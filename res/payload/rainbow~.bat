@echo off
setlocal

echo rainbow~

:the_RAINBOW_loop_XD
for %%i in (1,2,3,4,5,6,7,8,9,A,B,C,D,E,F) do (
	color %%i
)
goto the_RAINBOW_loop_XD

endlocal