#define _DEFAULT_SOURCE
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#define S(X) (X), sizeof(X)-1

int main(int argc, char* argv[]){
  if(argc == 2){
    if(!strcmp(argv[1], "--enable")){
      int fd = open("/proc/sys/fs/binfmt_misc/register", O_WRONLY|O_CREAT);
      if(fd == -1){
        perror("open failed");
        return 1;
      }
      if(write(fd, S((":shebang:M::#!::/proc/self/exe:FP\n"))) == -1){
        perror("write failed");
        return 1;
      }
      close(fd);
      return 0;
    }else if(!strcmp(argv[1], "--disable")){
      int fd = open("/proc/sys/fs/binfmt_misc/shebang", O_WRONLY);
      if(fd == -1){
        if(errno == ENOENT)
          return 0;
        perror("open failed");
        return 1;
      }
      if(write(fd, S(("-1\n"))) == -1){
        perror("write failed");
        return 1;
      }
      close(fd);
      return 0;
    }
  }
  if(argc < 3){
    fprintf(stderr, "Usage: shebang script argv0 [args...]\n");
    fprintf(stderr, "Usage: shebang --enable|--disable\n");
    return 2;
  }
  int fd = open(argv[1], O_RDONLY|O_CLOEXEC);
  if(fd == -1){
    fprintf(stderr, "open(\"%s\") failed: %d %s\n", argv[1], errno, strerror(errno));
    return 1;
  }
  // If this is a fifo or something, we don't want to do anything with it!
  if(!isfdtype(fd, S_IFREG) || lseek(fd, 0, SEEK_SET) == (off_t)-1){
    fprintf(stderr, "Refusing to process irregular file!\n");
    return 1;
  }
  FILE* exe = fdopen(fd, "rb");
  if(!exe){
    perror("fdopen failed\n");
    return 1;
  }
  static char shebang[4096];
  if(!fgets(shebang, sizeof(shebang), exe)){
    perror("fgets failed\n");
    return 1;
  }
  long len = ftell(exe);
  if(len == -1 || len > (long)sizeof(shebang)-1){
    fprintf(stderr, "ftell either returned an error or an impossible value\n");
    return 1;
  }
  if(len < 2 || shebang[0] != '#' || shebang[1] != '!'){
    fprintf(stderr, "File has no #! marker at the start\n");
    return 1;
  }
  if(len == (long)sizeof(shebang)-1 && shebang[len-1] != '\n'){
    fprintf(stderr, "shebang line is too long\n");
    return 1;
  }
  if(shebang[len-1] == '\n')
    shebang[len-1] = 0;
  char** nargv;
  // A page is usually about 4096 bytes long, and a pointer is usually up to 8 bytes. (256+256)*8=4096. Using malloca if larger to make sure there can't be any stack overflows missing guard pages.
  // Also, no need to free this later, this isn't a long lived program.
  nargv = argc-1 <= 256 ? alloca(sizeof(char[256+argc-1])) : malloc(sizeof(char[256+argc-1]));
  if(!nargv){
    perror("malloc failed");
    return 1;
  }
  size_t nargc = 0;
  bool onarg = false;
  char* it = shebang + 2;
  char* wit = it;
  for(bool eof=false; !eof; it++,wit++){
    char c = *it;
    if(c == '\\' && it[1] == 0)
      *it = c = 0;
    if(it != wit)
      *wit = c;
    if(!c) eof = true;
    if(c == '\\'){
      it += 1;
      *wit = *it;
      continue;
    }
    if(c == ' ' || c == 0){
      *it = 0;
      onarg = false;
    }else if(!onarg){
      onarg = true;
      if(nargc >= 256){
        fprintf(stderr, "Too many arguments in shebang line\n");
        return 1;
      }
      nargv[nargc++] = it;
    }
  }
  if(!nargc)
    nargv[nargc++] = "sh";
  nargv[nargc++] = argv[1];
  memcpy(&nargv[nargc], &argv[3], sizeof(char*[argc-3]));
  nargc += argc - 3;
  char* interp = nargv[0];
  nargv[0] = argv[2];
  nargv[nargc] = 0;
  fclose(exe);
  //puts(interp);
  //for(char**it=nargv; *it; it++) puts(*it);
  //return 0;
  int ret = execvp(interp, nargv);
  perror("execvp failed");
  return ret;
}
