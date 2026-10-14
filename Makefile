# VerdheimEngine Makefile
# 用法：mingw32-make       构建 engine.exe
#       mingw32-make clean 删除 engine.exe

CC = gcc
CFLAGS = -Wall

SRCS = Engine/EntryPoint/EntryPoint.c \
       Engine/InitializeCheck/InitializeCheck.c \
       Engine/3DSpaceDefinition/Space.c \
       Engine/3DSpaceDefinition/CameraRender.c \
       Engine/Render/SpriteDefinition.c \
       Engine/Render/CPU_Calculation.c \
       Engine/Render/Load_Sprite_From_Disk.c

engine.exe: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o engine.exe

clean:
	del engine.exe
