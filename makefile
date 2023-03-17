
CFLAGS += -flto -std=c99 -Wall -Wextra -pedantic
CFLAGS += -Oz -ffunction-sections -fdata-sections
LDFLAGS += -Wl,-gc-sections
LDFLAGS += --static

all: shebang

clean:
	rm -f shebang
