#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <cutils/properties.h>
#include <android/log.h>

#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "rebootcmd: ", __VA_ARGS__)

int main(int argc, char**argv, char *envp[])
{
	if(strcmp(argv[1], "recovery") == 0){
		//returns 1 if error
		ALOGD("Setting property sys.reboot.recovery return = %d",
				property_set("sys.reboot.recovery", "1"));
	}

	//Needed to allow the script to write to /boot/moboot.next
	sleep(1);
	return -1;
}
