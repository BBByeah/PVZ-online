package main

import (
 "bytes"
 "testing"
)

func TestObserverOutputFinishesPartialBeforeLatestSuccessor(t *testing.T) {
 s:=newServer(func(string,...any){})
 var o observerOutput
 data:=func(c byte)[]byte{return bytes.Repeat([]byte{c},100)}
 o.offer(s,[][]byte{data('A'),data('B'),data('C')})
 if got:=o.pop(s);len(got)!=100||got[0]!='A'{t.Fatal("first fragment")}
 o.offer(s,[][]byte{data('D'),data('E')})
 o.offer(s,[][]byte{data('F'),data('G')})
 for _,want:=range []byte("BCFG") {
  got:=o.pop(s);if len(got)!=100||got[0]!=want{t.Fatalf("partial frame interrupted: wanted %c",want)}
 }
 if o.pop(s)!=nil||s.observerMemory.Load()!=0{t.Fatal("output or memory retained")}
 o.offer(s,[][]byte{data('A'),data('B')});o.pop(s)
 o.offer(s,[][]byte{data('C')});o.clear(s)
 if s.observerMemory.Load()!=0{t.Fatal("disconnect retained pending frame")}
}
