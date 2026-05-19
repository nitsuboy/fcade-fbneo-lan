BINDIR = bin
BUILDDIR = build
SRCDIR = src
INCDIR = include
CC = gcc

CFLAGS = -I$(INCDIR) -O2 -Wall -Wextra -std=c99

ifeq ($(OS),Windows_NT)
    CC = C:/raylib/w64devkit/bin/gcc.exe
    LDFLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -lole32 
    TARGET = $(BINDIR)/fclauncher.exe
    MKDIR = mkdir
    RMDIR = rmdir /S /Q
else
    CFLAGS = -I$(INCDIR) -O2 -Wall -Wextra
    LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    TARGET = $(BINDIR)/fclauncher
    MKDIR = mkdir -p
    RMDIR = rm -rf
endif

OBJS = $(BUILDDIR)/main.o $(BUILDDIR)/config.o $(BUILDDIR)/process.o $(BUILDDIR)/roms.o

$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/app.h $(INCDIR)/config.h $(INCDIR)/process.h $(INCDIR)/roms.h | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR) $(BUILDDIR):
	-$(MKDIR) $@

clean:
	$(RMDIR) $(BUILDDIR)
	$(RMDIR) $(BINDIR)

.PHONY: clean