SRCDIR = src
INCDIR = include
BUILDDIR = build
BINDIR = bin
OBJS = $(BUILDDIR)/main.o $(BUILDDIR)/config.o $(BUILDDIR)/process.o $(BUILDDIR)/roms.o
CFLAGS = -O2 -Wall -Wextra -I$(INCDIR)
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
$(BINDIR)/fclauncher: $(OBJS) | $(BINDIR)
	gcc $^ -o $@ $(LDFLAGS)
$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/app.h $(INCDIR)/raygui.h $(INCDIR)/config.h $(INCDIR)/process.h $(INCDIR)/roms.h | $(BUILDDIR)
	gcc $(CFLAGS) -c $< -o $@
$(BINDIR) $(BUILDDIR):
	mkdir -p $@
clean:
	rm -rf $(BUILDDIR) $(BINDIR)
.PHONY: clean