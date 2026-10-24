package main
import("bytes";"encoding/binary";"hash/crc32";"testing";"time")
func obsControl(src,dst,op byte,watch,stream,epoch uint32)[]byte{p:=make([]byte,30);p[0]=src;p[1]=dst;p[2]=2;p[3]=op;binary.LittleEndian.PutUint32(p[6:],watch);binary.LittleEndian.PutUint32(p[10:],stream);binary.LittleEndian.PutUint32(p[14:],epoch);binary.LittleEndian.PutUint32(p[18:],observerRevision);binary.LittleEndian.PutUint32(p[22:],24*1024);binary.LittleEndian.PutUint32(p[26:],1500);return p}
func obsParts(src,dst byte,watch,stream,epoch,seq uint32,data []byte)[][]byte{count:=(len(data)+215)/216;out:=[][]byte{};for i:=0;i<count;i++{n:=len(data)-i*216;if n>216{n=216};p:=make([]byte,40+n);p[0]=src;p[1]=dst;p[2]=2;p[3]=1;binary.LittleEndian.PutUint32(p[4:],watch);binary.LittleEndian.PutUint32(p[8:],stream);binary.LittleEndian.PutUint32(p[12:],seq);binary.LittleEndian.PutUint32(p[16:],1500);binary.LittleEndian.PutUint32(p[20:],epoch);binary.LittleEndian.PutUint16(p[24:],uint16(i));binary.LittleEndian.PutUint16(p[26:],uint16(count));binary.LittleEndian.PutUint32(p[28:],uint32(n));binary.LittleEndian.PutUint32(p[32:],binary.LittleEndian.Uint32(data[1:]));binary.LittleEndian.PutUint32(p[36:],crc32.ChecksumIEEE(data));copy(p[40:],data[i*216:i*216+n]);out=append(out,p)};return out}
func obsData()[]byte{p:=make([]byte,1200);p[0]=1;binary.LittleEndian.PutUint32(p[1:],4096);for i:=5;i<len(p);i++{p[i]=byte(i)};return p}
func obsNext(c *testClient,typ uint16)wireFrame{for i:=0;i<30;i++{f:=c.next(time.Second);if f.typ==typ{return f}};c.t.Fatal("expected observer frame absent");return wireFrame{}}
func TestObserverV2ActualTCPAndGameplayPriority(t *testing.T){
 s,addr:=startTestServer(t,nil);h,g:=newTestClient(t,addr),newTestClient(t,addr);code:=decodeWelcome(t,h.create(1,94,"source")).code;g.join(1,94,"viewer",code);h.next(time.Second)
 capReq:=make([]byte,4);binary.LittleEndian.PutUint32(capReq,observerRevision)
 for _,c:=range []*testClient{h,g}{c.send(0xF023,capReq);if f:=obsNext(c,0xF023);len(f.payload)!=8{t.Fatal("capability")}}
 g.send(30,obsControl(2,1,1,101,1,1));if f:=obsNext(h,30);f.payload[3]!=1{t.Fatal("begin")}
 h.send(30,obsControl(1,2,2,101,7,1));if f:=g.next(time.Second);f.typ!=30||f.payload[10]!=7{t.Fatal("accept")}
 parts:=obsParts(1,2,101,7,1,1,obsData());for _,p:=range parts{h.send(31,p)}
 var actual []byte;for range parts{f:=g.next(time.Second);if f.typ!=31{t.Fatalf("frame %x",f.typ)};actual=append(actual,f.payload[40:]...)};if !bytes.Equal(actual,obsData()){t.Fatal("full frame changed")}
 // A source cannot impersonate a viewer, mix a previous stream, or bypass STOP.
 for _,p:=range obsParts(2,2,101,7,1,2,obsData()){h.send(31,p)};g.expectNothing(25*time.Millisecond)
 for _,p:=range obsParts(1,2,101,8,1,2,obsData()){h.send(31,p)};g.expectNothing(25*time.Millisecond)
 g.send(30,obsControl(2,1,4,101,7,1));if h.next(time.Second).typ!=30{t.Fatal("stop")}
 for _,p:=range obsParts(1,2,101,7,1,3,obsData()){h.send(31,p)};g.expectNothing(25*time.Millisecond)
 // Gameplay still crosses the actual same connections after invalid display.
 h.send(16,[]byte{1,2,1});if f:=g.next(time.Second);f.typ!=16{t.Fatal("gameplay blocked")}
 _=s
}
func TestObserverV2AuthorizationAssemblyAndBudget(t *testing.T){
 s:=newServer(func(string,...any){});r:=&Room{};a:=&Conn{srv:s,seat:1,build:94,observerCap:true,send:make(chan outMsg,64),done:make(chan struct{})};b:=&Conn{srv:s,seat:2,build:94,observerCap:true,send:make(chan outMsg,64),done:make(chan struct{})};a.room.Store(r);b.room.Store(r);r.bySeat[1]=a;r.bySeat[2]=b
 begin:=func(){s.observerLocked(r,b,a,30,obsControl(2,1,1,10,1,1),false);s.observerLocked(r,a,b,30,obsControl(1,2,2,10,9,1),false)}
 r.roundWaitMask=2;begin();if r.observer[2]!=nil{t.Fatal("waiting source accepted")};r.roundWaitMask=4;begin();if r.observer[2]==nil{t.Fatal("waiting viewer rejected")};r.roundWaitMask=0
 parts:=obsParts(1,2,10,9,1,1,obsData());s.observerLocked(r,a,b,31,parts[0],false);s.observerLocked(r,a,b,31,parts[2],false);if s.observerMemory.Load()!=0{t.Fatal("out of order retained")}
 for i:=range parts{p:=append([]byte{},parts[i]...);if i==len(parts)-1{p[len(p)-1]^=1};s.observerLocked(r,a,b,31,p,false)};if b.observer.bytes!=0||s.observerMemory.Load()!=0{t.Fatal("CRC failure published")}
 for _,p:=range parts{s.observerLocked(r,a,b,31,p,false)};if b.observer.bytes==0||len(b.send)!=1{t.Fatal("display entered gameplay queue or missing")};b.observer.clear(s)
 for _,p:=range parts{s.observerLocked(r,a,b,31,p,false)};if b.observer.bytes!=0{t.Fatal("replay accepted")}
 begin();r.observer[2].touched=time.Now().Add(-5*time.Second);s.sweepRoom(r,time.Now());if r.observer[2]!=nil||s.observerMemory.Load()!=0{t.Fatal("timeout retained memory")}
 begin();s.observerBusy.Store(true);for _,p:=range parts{s.observerLocked(r,a,b,31,p,false)};if b.observer.bytes!=0{t.Fatal("busy display queued")};s.observerBusy.Store(false)
 // Latest full frame replacement is bounded, while critical has all 64 slots.
 for i:=0;i<100;i++{b.observer.offer(s,[][]byte{make([]byte,10000)})};if b.observer.bytes!=10000||s.observerMemory.Load()!=10000{t.Fatal("replacement leaked")};s.clearObserverConn(r,b);if s.observerMemory.Load()!=0{t.Fatal("disconnect leaked")}
}
