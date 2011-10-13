#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#define RUNCMD "/system/bin/boot_webos"

int main(int argc, char**argv, char *envp[])
{
	char *newargv[] = { RUNCMD, argv[1], NULL };
	setuid(0);

	execve(RUNCMD, newargv, envp);

	return -1;
}
