

@echo off
for %%i in (*.vert *.frag *.comp) do "C:\VulkanSDK\1.2.162.0\Bin\glslangValidator.exe " -V "%%~i" -o "../spv/%%~i.spv"