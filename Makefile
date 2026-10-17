# VerdheimEngine Makefile
# 用法：mingw32-make       构建 engine.exe
#       mingw32-make clean 删除 engine.exe

CC = gcc
CFLAGS = -Wall -mwindows
LIBS = -luser32 -lgdi32

SRCS = Engine/EntryPoint/EntryPoint.c \
       Engine/EntryPoint/HWND/HWND.c \
       Engine/InitializeCheck/InitializeCheck.c \
       Engine/3DSpaceDefinition/Space.c \
       Engine/3DSpaceDefinition/CameraRender.c \
       Engine/Render/SpriteDefinition.c \
       Engine/Render/CPU_Calculation.c \
       Engine/Render/Load_Sprite_From_Disk.c \
       Engine/Render/Render_Pictures.c

engine.exe: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o engine.exe $(LIBS)

clean:
	del engine.exe
