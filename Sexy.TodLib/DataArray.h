#ifndef __DATAARRAY_H__
#define __DATAARRAY_H__

#include <new.h>
#include <string.h>
#include "TodDebug.h"
#include "TodCommon.h"

enum 
{
	DATA_ARRAY_INDEX_MASK = 65535,
	DATA_ARRAY_KEY_MASK = -65536,
	DATA_ARRAY_KEY_SHIFT = 16,
	DATA_ARRAY_MAX_SIZE = 65536,
	DATA_ARRAY_KEY_FIRST = 1
};

template <typename T> class DataArray
{
public:
	class DataArrayItem
	{
	public:
		T					mItem;
		unsigned int		mID;
	};
	
public:
	DataArrayItem*			mBlock;
	unsigned int			mMaxUsedCount;
	unsigned int			mMaxSize;
	unsigned int			mFreeListHead;
	unsigned int			mSize;
	unsigned int			mNextKey;
	const char*				mName;

public:
	DataArray<T>()
	{
		mBlock = nullptr;
		mMaxUsedCount = 0U;
		mMaxSize = 0U;
		mFreeListHead = 0U;
		mSize = 0U;
		mNextKey = 1U;
		mName = nullptr;
	}

	~DataArray<T>()
	{
		DataArrayDispose();
	}

	void DataArrayInitialize(unsigned int theMaxSize, const char* theName)
	{
		TOD_ASSERT(mBlock == nullptr);
		mBlock = (DataArrayItem*)operator new(sizeof(DataArrayItem) * theMaxSize);
		mMaxSize = theMaxSize;
		mNextKey = 1001U;
		mName = theName;
	}

	void DataArrayDispose()
	{
		if (mBlock != nullptr)
		{
			DataArrayFreeAll();
			operator delete(mBlock);
			mBlock = nullptr;
			mMaxUsedCount = 0U;
			mMaxSize = 0U;
			mFreeListHead = 0U;
			mSize = 0U;
			mName = nullptr;
		}
	}

	void DataArrayFree(T* theItem)
	{
		DataArrayItem* aItem = (DataArrayItem*)theItem;
		TOD_ASSERT(DataArrayGet(aItem->mID) == theItem, "Failed: DataArrayFree(0x%x) in %s", theItem, mName);
		theItem->~T();
		unsigned int anId = aItem->mID & DATA_ARRAY_INDEX_MASK;
		aItem->mID = mFreeListHead;
		mFreeListHead = anId;
		mSize--;
	}

	void DataArrayFreeAll()
	{
		T* aItem = nullptr;
		while (IterateNext(aItem))
			DataArrayFree(aItem);

		mFreeListHead = 0U;
		mMaxUsedCount = 0U;
	}

	inline unsigned int DataArrayGetID(T* theItem)
	{
		DataArrayItem* aItem = (DataArrayItem*)theItem;
		TOD_ASSERT(DataArrayGet(aItem->mID) == theItem, "Failed: DataArrayGetID(0x%x) for %s", theItem, mName);
		return aItem->mID;
	}

	bool IterateNext(T*& theItem)
	{
		DataArray<T>::DataArrayItem* aItem = (DataArray<T>::DataArrayItem*)theItem;
		if (aItem == nullptr)
			aItem = &mBlock[0];
		else
			aItem++;

		DataArray<T>::DataArrayItem* aLast = &mBlock[mMaxUsedCount];
		while ((intptr_t)aItem < (intptr_t)aLast)
		{
			if (aItem->mID & DATA_ARRAY_KEY_MASK)
			{
				theItem = (T*)aItem;
				return true;
			}
			aItem++;
		}
		return false;
	}

	// Checked allocation is opt-in until every legacy caller handles failure.
	bool DataArrayCanAllocate(unsigned int theCount = 1) const
	{
		if (mMaxUsedCount > mMaxSize || mSize > mMaxUsedCount || mFreeListHead > mMaxUsedCount)
			return false;
		if ((mFreeListHead == mMaxUsedCount) != (mSize == mMaxUsedCount)) return false;
		return theCount <= mMaxSize - mSize;
	}

	T* DataArrayTryAlloc()
	{
		if (!DataArrayCanAllocate()) return nullptr;
		if (!mBlock) return nullptr;
		if (mFreeListHead < mMaxUsedCount &&
			(mBlock[mFreeListHead].mID > mMaxUsedCount || mBlock[mFreeListHead].mID == mFreeListHead)) return nullptr;
		return DataArrayAlloc();
	}

	// Validate loaded IDs/free links before callers can iterate or allocate.
	bool DataArrayValidateState() const
	{
		if (!DataArrayCanAllocate(0) || mNextKey == 0 || mNextKey >= DATA_ARRAY_MAX_SIZE) return false;
		unsigned int aLive = 0;
		for (unsigned int i = 0; i < mMaxUsedCount; ++i)
		{
			const unsigned int aId = mBlock[i].mID;
			if (aId & DATA_ARRAY_KEY_MASK)
			{
				if ((aId & DATA_ARRAY_INDEX_MASK) != i) return false;
				++aLive;
			}
			else if (aId > mMaxUsedCount) return false;
		}
		if (aLive != mSize) return false;
		unsigned int aNext = mFreeListHead;
		for (unsigned int n = 0; n < mMaxUsedCount - mSize; ++n)
		{
			if (aNext >= mMaxUsedCount) return false;
			aNext = mBlock[aNext].mID;
		}
		return aNext == mMaxUsedCount;
	}

	T* DataArrayAlloc()
	{
		TOD_ASSERT(mSize < mMaxSize, "Data array full: %s", mName);
		TOD_ASSERT(mFreeListHead <= mMaxUsedCount, "DataArrayAlloc error in %s", mName);
		unsigned int aNext = mMaxUsedCount;
		if (mFreeListHead == mMaxUsedCount)
			mFreeListHead = ++mMaxUsedCount;
		else
		{
			aNext = mFreeListHead;
			mFreeListHead = mBlock[mFreeListHead].mID;
		}

		DataArray<T>::DataArrayItem* aNewItem = &mBlock[aNext];
		memset(aNewItem, 0, sizeof(DataArrayItem));
		aNewItem->mID = (mNextKey++ << DATA_ARRAY_KEY_SHIFT) | aNext;
		if (mNextKey == DATA_ARRAY_MAX_SIZE) mNextKey = 1;
		mSize++;

		new (aNewItem)T();
		return (T*)aNewItem;
	}

	T* DataArrayTryToGet(unsigned int theId)
	{
		if (!theId || !(theId & DATA_ARRAY_KEY_MASK) || (theId & DATA_ARRAY_INDEX_MASK) >= mMaxUsedCount)
			return nullptr;

		DataArrayItem* aBlock = &mBlock[theId & DATA_ARRAY_INDEX_MASK];
		return (aBlock->mID == theId) ? &aBlock->mItem : nullptr;
	}

	T* DataArrayGet(unsigned int theId)
	{
		TOD_ASSERT(DataArrayTryToGet(theId) != nullptr, "Failed: DataArrayGet(0x%x) for %s", theId, mName);
		return &mBlock[theId & DATA_ARRAY_INDEX_MASK].mItem;
	}
};

#endif
