#ifndef __NEWOPTIONSDIALOG_H__
#define __NEWOPTIONSDIALOG_H__

#include "widget/Dialog.h"
#include "RunInfoView.h"
#include "MusicSelectionView.h"
#include "RunSettingsView.h"
#include "widget/SliderListener.h"
#include "widget/CheckboxListener.h"

class LawnApp;
class LawnStoneButton;
class NewLawnButton;
namespace Sexy
{
	class Slider;
	class Checkbox;
};

class NewOptionsDialog : public Sexy::Dialog, public Sexy::SliderListener, public Sexy::CheckboxListener
{
protected:
	enum
	{
		NewOptionsDialog_Almanac,
		NewOptionsDialog_MainMenu,
		NewOptionsDialog_Restart,
		NewOptionsDialog_Update,
		NewOptionsDialog_MusicVolume,
		NewOptionsDialog_SoundVolume,
		NewOptionsDialog_Fullscreen,
		NewOptionsDialog_HardwareAcceleration,
        NewOptionsDialog_Help,
	};

public:
	LawnApp*				mApp;								//+0x158
	Sexy::Slider*			mMusicVolumeSlider;					//+0x15C
	Sexy::Slider*			mSfxVolumeSlider;					//+0x160
	Sexy::Checkbox*			mFullscreenCheckbox;				//+0x164
	Sexy::Checkbox*			mHardwareAccelerationCheckbox;		//+0x168
	LawnStoneButton*		mAlmanacButton;						//+0x16C
	LawnStoneButton*		mBackToMainButton;					//+0x170
	LawnStoneButton*		mRestartButton;						//+0x174
	NewLawnButton*			mBackToGameButton;					//+0x178
	bool					mFromGameSelector;					//+0x17C
    bool mRunInfoOpen;
    bool mRunInfoWidgetVis[9]{};
    LawnStoneButton* mHelpButton;

public:
    static constexpr int PAUSE_WIDTH = 600;
    static constexpr int PAUSE_HEIGHT = 560;
    RunInfoView mRunInfoView;
    MusicSelectionView mMusicView;
    RunSettingsView mRunSettingsView;
    bool mRunSettingsOpen = false;
    bool mRunSettingsEntryDown = false;
    Sexy::Rect RunSettingsEntryRect();
    void OpenRunSettings();
    bool mMusicOpen = false;
    bool mMusicEntryDown = false;
    Sexy::Rect MusicEntryRect();
    void OpenOptionsPage(bool music, bool settings = false);
    void OpenMusic();
    void MusicAction(int action);
    Sexy::Rect mOptionsRect;
    int mRunInfoDown = -1;
    void MouseUp(int x, int y, int theClickCount) override;
    void LostFocus() override;
	NewOptionsDialog(LawnApp* theApp, bool theFromGameSelector);
	~NewOptionsDialog();

	int						GetPreferredHeight(int theWidth);
	void					AddedToManager(Sexy::WidgetManager* theWidgetManager);
	void					RemovedFromManager(Sexy::WidgetManager* theWidgetManager);
	void					Resize(int theX, int theY, int theWidth, int theHeight);
	void					Draw(Sexy::Graphics* g);
	void					SliderVal(int theId, double theVal);
	void					CheckboxChecked(int theId, bool checked);
	void					ButtonPress(int theId);
	void					ButtonDepress(int theId);
	void					KeyDown(Sexy::KeyCode theKey);
	void					MouseDown(int x, int y, int theClickCount);
	void					MouseWheel(int theDelta);

	bool					RunInfoAvailable();
	Sexy::Rect				RunInfoEntryRect();
	void					RunInfoFlipPage(int theDelta);
	void					OpenRunInfo();
	void					CloseRunInfo();
	void					DrawRunInfo(Sexy::Graphics* g);
};

#endif
