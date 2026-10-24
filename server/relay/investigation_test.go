package main
import("flag";"os";"path/filepath";"strconv";"testing";"time")
var probeDir=flag.String("probe-dir","","isolated fixture files")
func TestSessionInvestigation(t *testing.T){ if *probeDir==""{t.Skip("explicit owned fixture only")};
 h,err:=startHostedRelay("127.0.0.1:0",true,t.Logf);if err!=nil{t.Fatal(err)};defer h.Close()
 if err=os.WriteFile(filepath.Join(*probeDir,"port"),[]byte(strconv.Itoa(h.server.gamePort)),0600);err!=nil{t.Fatal(err)}
 end:=time.Now().Add(60*time.Second)
 for time.Now().Before(end){if _,err=os.Stat(filepath.Join(*probeDir,"stop"));err==nil{return};time.Sleep(20*time.Millisecond)}
 t.Fatal("owned client fixture did not finish")
}
