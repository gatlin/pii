CC := gcc # This is the main compiler
# CC := clang --analyze # and comment out the linker last line for sanity
SRCDIR := src
BUILDDIR := build
TARGET := bin/pii

GREEN  := $(shell tput -Txterm setaf 2)
YELLOW := $(shell tput -Txterm setaf 3)
WHITE  := $(shell tput -Txterm setaf 7)
RESET  := $(shell tput -Txterm sgr0)

# Anything in the source folder will be compiled as long as it has SRCEXT
# extension.
SRCEXT := c
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
PURPLE_CFLAGS := $(shell pkg-config --cflags purple)
PURPLE_LIBS := $(shell pkg-config --libs purple)
CFLAGS := -g -Wall $(PURPLE_CFLAGS)
LIB := -L lib $(PURPLE_LIBS)
INC := -I include

## Default: build the pii executable.
pii: $(OBJECTS)
	@mkdir -p bin
	@echo " Linking...";
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)

## Build and run with debug flags turned on.
debug: pii
	G_DEBUG=1 bin/pii

## Build targets
$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

## Clean up build artifacts.
clean:
	@echo " Cleaning...";
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

# Tests
#tester:
#  $(CC) $(CFLAGS) test/tester.cpp $(INC) $(LIB) -o bin/tester

# Spikes
#ticket:
#  $(CC) $(CFLAGS) spikes/ticket.cpp $(INC) $(LIB) -o bin/ticket

.PHONY: clean pii

## Print help message.
help:
		@echo ''
		@echo 'Usage:'
		@echo '  ${YELLOW}make${RESET} ${GREEN}<target>${RESET}'
		@echo ''
		@echo 'Targets:'
		@awk '/^[a-zA-Z\-\_0-9]+:/ { \
				helpMessage = match(lastLine, /^## (.*)/); \
				if (helpMessage) { \
						helpCommand = substr($$1, 0, index($$1, ":")-1); \
						helpMessage = substr(lastLine, RSTART + 3, RLENGTH); \
						printf "  ${YELLOW}%-$(TARGET_MAX_CHAR_NUM)s${RESET} \n\t${GREEN}%s${RESET}\n", helpCommand, helpMessage; \
				} \
		} \
		{ lastLine = $$0 }' $(MAKEFILE_LIST)
