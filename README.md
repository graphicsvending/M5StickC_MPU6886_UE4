# M5StickC_MPU6886_UE4

Hardware: M5StickC, wifi
Software: Arduino IDE, Unreal Engine 4.25

Sending UDP packet...(roll, pitch, yaw data) from M5StickC MPU6886 to Unreal Engine 4(LiveLink) for CG virtual tracking... 
my Arduino code is sending range +/- 90 degree of Roll & Pitch... but Yaw is drifting(MPU6886 lack of magnetometer)

I am still fine-tuning the code for getting full range +/-360 degree and less jitter output... 
seeking for help for better quality on MPU6886 data output... please feel free to input your code on this project...Thanks




1. Download and Install Unreal Engine
https://www.unrealengine.com/en-US/download

2. Download and Install LONET Unreal Engine Plugin
https://www.loledvirtual.com/documentation/docs/unrealreleases

3. Check out this:
https://www.loledvirtual.com/documentation/docs/lonetdev

4. Download the Unreal Engine Project files
https://drive.google.com/drive/folders/12sgE5-MO9ItesXXhqqAgh_OQntE_AEqo?usp=sharing

5. Arduino Code from:
https://github.com/graphicsvending/M5StickC_MPU6886_UE4
