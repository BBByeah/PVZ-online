#include "Lawn/Online/NetSession.h"
#include "LawnApp.h"
#include "Lawn/CoopPlay.h"
#include "Lawn/Plant.h"
#include "Lawn/Online/CoopProtocol.h"
#include "Common.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
static int failed=0;
static void check(bool value,const char* label){std::printf("%s %s\n",value?"PASS":"FAIL",label);failed+=!value;}
int main(){
 auto* app=new LawnApp();gLawnApp=app;
 std::filesystem::create_directories("android-artifacts/coop52/test-data/userdata");
 Sexy::SetAppDataFolder("android-artifacts/coop52/test-data/");
 RunState run;run.StartNew(520123,RunState::RUN_MODE_FULL);app->mRunState=&run;
 int pea=RUN_BUFF_COUNT+RUN_UPGRADE_PEASHOOTER;
 run.mBuffs={{(unsigned short)pea,2},{RUN_BUFF_ROOTED,1}};
 run.mLoanPlant=SEED_PEASHOOTER;run.mLoanLevel=0;run.mLoanBuffs={{(unsigned short)pea,3}};
 check(run.GetBuffCount(pea)==2 && run.GetEffectiveBuffCount(pea)==3,"loan maximum preserves permanent stacks");
 check(app->RunPlantUpgradeCount(SEED_PEASHOOTER)==3,"effect consumer uses temporary maximum");
 check(run.GetEffectiveBuffCount(RUN_BUFF_ROOTED)==1,"global buffs remain local");
 check(run.GetUsablePool().size()==run.mPool.size(),"same plant does not duplicate a bank card");
 run.mLoanPlant=SEED_CACTUS;run.mLoanBuffs.clear();
 check(run.HasUsablePlant(SEED_CACTUS)&&!run.HasPlant(SEED_CACTUS),"temporary availability does not unlock permanent pool");
 check(run.GetUsablePool().size()==run.mPool.size()+1,"missing borrowed plant is added to playable pool");
 check(!run.ValidateLoan(SEED_CACTUS,{{RUN_BUFF_ROOTED,4}}),"global traits rejected from snapshot");
 int base=RUN_BUFF_COUNT+RUN_PLANT_UPGRADE_COUNT;
 run.mBuffs.push_back({(unsigned short)(RUN_BUFF_COUNT+RUN_UPGRADE_WALLNUT),1});
 check(!run.ValidateLoan(SEED_WALLNUT,{{(unsigned short)(base+RUN_BUFF2_NUT_BOWLING),1}}),"mutually exclusive mechanisms rejected");
 run.mLoanPlant=SEED_PEASHOOTER;run.mLoanBuffs={{(unsigned short)pea,3}};
 check(run.Save(52001),"save v16 loan snapshot");RunState loaded;
 check(loaded.Load(52001)&&loaded.GetBuffCount(pea)==2&&loaded.GetEffectiveBuffCount(pea)==3,"reload preserves loan separately");
 std::ifstream in(RunState::GetCheckpointName(52001),std::ios::binary);std::vector<char> data((std::istreambuf_iterator<char>(in)),{});in.close();
 data.resize(data.size()-18);data[4]=15;data[5]=0;
 std::ofstream out(RunState::GetCheckpointName(52002),std::ios::binary);out.write(data.data(),data.size());out.close();RunState old;
 check(old.Load(52002)&&old.mLoanPlant==-1&&old.GetEffectiveBuffCount(pea)==2,"v15 checkpoint loads without temporary effects");
 CoopPlay coop(app);coop.ExpireLoan();
 check(run.mLoanPlant==-1&&app->RunPlantUpgradeCount(SEED_PEASHOOTER)==2,"explicit return restores own effect consumer");
 run.mLoanPlant=SEED_CACTUS;run.mLoanLevel=0;run.AdvanceLevel();
 check(!run.HasUsablePlant(SEED_CACTUS)&&run.GetEffectiveBuffCount(pea)==2,"stage change expires card and effects");
 CoopWire::Event e;e.op=CoopWire::OFFER;e.src=1;e.dst=2;e.sender=0x1122334455667788ULL;e.target=0xaabbccddeeff0011ULL;e.run=99;e.level=500;e.serial=4;e.request=3;e.plant=SEED_PEASHOOTER;e.count=1;e.ids[0]=pea;e.stacks[0]=3;
 uint8_t bytes[256];int size=CoopWire::Encode(bytes,e);CoopWire::Event decoded;
 check(CoopWire::Decode(bytes,size,decoded)&&decoded.target==e.target&&decoded.sender==e.sender&&decoded.level==500&&decoded.stacks[0]==3,"bounded wire preserves identity, scope and snapshot");
 check(!CoopWire::Decode(bytes,size-1,decoded),"truncated event rejected");
 bytes[4]=17;check(!CoopWire::Decode(bytes,size,decoded),"oversized trait count rejected");
 e.count=2;e.ids[1]=e.ids[0];e.stacks[1]=1;size=CoopWire::Encode(bytes,e);
 check(!CoopWire::Decode(bytes,size,decoded),"duplicate trait IDs rejected");

 NetSession host,client;
 bool opened=host.StartHost(28252)&&client.StartJoin("127.0.0.1",28252);
 for(int i=0;i<200&&(!host.IsConnected()||!client.IsConnected());++i){host.Update();client.Update();Sleep(2);}
 check(opened&&host.IsConnected()&&client.IsConnected(),"two native sessions connect");
 CoopWire::Event hello;hello.op=CoopWire::HELLO;
 host.SendCoop(hello);client.SendCoop(hello);
 for(int i=0;i<25;++i){host.Update();client.Update();Sleep(2);}
 check(host.FindCoopSeat(client.mCoopToken)==client.GetLocalSeat()&&client.FindCoopSeat(host.mCoopToken)==host.GetLocalSeat(),"identity advertisement binds actual peers");
 CoopWire::Event drop;while(host.TakeCoop(drop)){}while(client.TakeCoop(drop)){}
 e.op=CoopWire::ASK;e.count=0;e.target=host.mCoopToken;e.serial=700;e.run=520123;e.level=500;
 client.SendCoop(e,host.GetLocalSeat());
 for(int i=0;i<25;++i){host.Update();client.Update();Sleep(2);}
 bool received=host.TakeCoop(drop);
 check(received&&drop.sender==client.mCoopToken&&drop.request==e.request&&drop.serial==700&&drop.level==500,"loan request traverses real session transport");
 e.target=host.mCoopToken^1;e.serial=701;client.SendCoop(e,host.GetLocalSeat());
 for(int i=0;i<25;++i){host.Update();client.Update();Sleep(2);}
 check(!host.TakeCoop(drop),"stale target identity rejected before game handler");
 std::printf("failures=%d\n",failed);return failed?1:0;
}
