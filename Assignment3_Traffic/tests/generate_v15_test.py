from pathlib import Path
p=Path(__file__).resolve().parent
s=(p.parent/'ESP32_Assignment2/ESP32_Assignment2.ino').read_text()
old=(p/'vendor_defaults_generated.cpp').read_text()
a=old[:old.index('bool cameraInit(){')]+ '\nbool wbBusy=false;\n'
b=s[s.index('bool cameraInit(){'):s.index('void loadCalibration(')]
c=old[old.index('int main(){'):].replace('V12 vendor startup','V16 SVGA startup')
(p/'camera_v15_generated.cpp').write_text(a+b+c)
block=s[s.index(' if(!calibrationReady||calibration.getInt("a3v15",0)!=1)'):s.index(' // V18 replaces',s.index('// One-time migration'))]
(p/'migration_v15_generated.cpp').write_text('''#include <cassert>
#include <cstring>
#include <cstdio>
#include <cmath>
#include "../ESP32_Assignment2/CalibrationStore.h"
struct DriveSettings {int sc=25,bl=600,bh=850,ra=90,sa=180,ct=1500,gs=1;}driveSettings;
CalibrationStore savedCalibration;
bool calibrationReady=true;
struct Prefs {int marker=0;bool fail=false;int getInt(const char*,int){return marker;}size_t putBytes(const char*,const void*,size_t n){return fail?0:n;}void putInt(const char*,int n){marker=n;}}calibration;
void migrate(){
'''+block+'''}
int main(){savedCalibration.light.width=40;savedCalibration.light.imageWidth=400;savedCalibration.light.cm=50;savedCalibration.light.resolution=5;
 migrate();assert(savedCalibration.stop.valid()&&savedCalibration.stop.width==96&&savedCalibration.stop.imageWidth==400&&savedCalibration.stop.cm==10&&savedCalibration.stop.resolution==5);
 assert(std::fabs(savedCalibration.stop.k()*400/96-10)<0.001f);assert(driveSettings.ra==150&&driveSettings.sa==300&&driveSettings.gs==1&&savedCalibration.light.cm==50&&calibration.marker==1);
 savedCalibration.stop={};driveSettings.ra=120;migrate();assert(!savedCalibration.stop.valid()&&driveSettings.ra==120);
 calibration.marker=0;calibration.fail=true;migrate();assert(calibration.marker==0&&savedCalibration.stop.valid());
 puts("PASS V15 one-time STOP profile, distance scale, wider scan defaults, preserve light/sign, retain later clear/settings, retry failed persistence.");}
''')
