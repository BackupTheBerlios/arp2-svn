
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/arp2_syscall.h>

#define O_ACCMODE          0003
#define O_RDONLY             00
#define O_WRONLY             01
#define O_RDWR               02

struct Library* ARP2_SysCallBase;

int main(void) {
  ARP2_SysCallBase = OpenResource(ARP2_SYSCALL_NAME);

  if (ARP2_SysCallBase != NULL) {
    LONG fd = arp2sys_open("/etc/passwd", O_RDONLY, 0);
    PrintFault(IoErr(), "arp2sys_open");
    Printf("Opened file %ld\n", fd);

    if (fd != -1) {
      char buf[512];

      LONG count = arp2sys_read(fd, buf, sizeof (buf) - 1);
      buf[511] = 0;

      PrintFault(IoErr(), "arp2sys_read");
      Printf("Read %ld bytes.\n", count);
      PutStr(buf);
      PutStr("\n");
    }

    arp2sys_close(fd);
    PrintFault(IoErr(), "arp2sys_close");
    Printf("Closed file %ld\n", fd);
  }

  return 0;
}
