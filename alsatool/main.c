#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sound/asound.h>

struct snd_ctl_elem_id *elements;
int nelements;

int get_elem_list( int fd, struct snd_ctl_elem_list* list )
{

	int i = 0;
	struct snd_ctl_elem_id* pid;
        if (ioctl( fd, SNDRV_CTL_IOCTL_ELEM_LIST, list) < 0) {		
		printf( " SNDRV_CTL_IOCTL_ELEM_LIST fail\n" );
		return -1;
	}

#if 0
	
	printf( " offset  is %d\n", list ->offset );
	printf( " sapce   is %d\n", list ->space );
	printf( " used    is %d\n", list ->used );
	printf( " count   is %d\n", list ->count );
	printf( " reserv  is %s\n", list ->reserved );

	pid = list ->pids;
	for( i = 0; i < list ->used; i++ ) {
		printf( " *****************************************\n" );
		printf( " numid    is %d\n", pid ->numid );
		printf( " iface    is %d\n", pid ->iface );
		printf( " device   is %d\n", pid ->device );
		printf( " name     is %s\n", pid ->name );
		printf( " index    is %d\n", pid ->index );
		pid++;
	
	}
#endif
	return 0;
}

struct snd_ctl_elem_id* get_id(char *name)
{
	int i;
	for( i = 0; i < nelements; i++ ) {
		if(strcmp(elements[i].name,name)==0)
			return &elements[i];
	}
	printf("Error unable to locate mixer control: %s\n", name);
	return 0;
}

int write_elem(int fd,  struct snd_ctl_elem_id* id, int d0, int d1, int d2)
{
        struct snd_ctl_elem_value control;
	
	control.id = *id;
	control.value.integer.value[0] = d0;
	control.value.integer.value[1] = d1;
	control.value.integer.value[2] = d2;
        if (ioctl( fd,SNDRV_CTL_IOCTL_ELEM_WRITE, &control ) < 0) {		
		printf( " SNDRV_CTL_IOCTL_ELEM_WRITE fail\n");
		return -1;
	}

	return 0;
}

int main(int argc, char** argv)
{
	struct snd_ctl_elem_list elem_list;
	struct snd_ctl_elem_id *pcm_playback_id;
	struct snd_ctl_elem_id *pcm_capture_id;
	struct snd_ctl_elem_id *speaker_stereo_rx_id;
	struct snd_ctl_elem_id *speaker_mono_tx_id;
	struct snd_ctl_elem_id *line1mix_id;
	struct snd_ctl_elem_id *line1outn_id;
	struct snd_ctl_elem_id *line1outp_id;
	struct snd_ctl_elem_id *line2mix_id;
	struct snd_ctl_elem_id *line2outn_id;
	struct snd_ctl_elem_id *line2outp_id;
	struct snd_ctl_elem_id *leftdacmix_id;
	struct snd_ctl_elem_id *rightdacmix_id;
	struct snd_ctl_elem_id *aif2adc_id;
	struct snd_ctl_elem_id *aif2adcvol_id;
	struct snd_ctl_elem_id *aif2adcr_id;
	struct snd_ctl_elem_id *aif2dacl_id;
	struct snd_ctl_elem_id *dac2vol_id;
	struct snd_ctl_elem_id *in1pgan_id;
	struct snd_ctl_elem_id *in1pgap_id;
	struct snd_ctl_elem_id *mixinl_id;
	struct snd_ctl_elem_id *in1l_id;
	struct snd_ctl_elem_id *in1lvol_id;
	struct snd_ctl_elem_id *in1lzc_id;
	struct snd_ctl_elem_id *mixinl1_id;
	struct snd_ctl_elem_id *mixinlvol_id;
	struct snd_ctl_elem_id *outvol_id;
	struct snd_ctl_elem_id *dac1aifl_id;
	struct snd_ctl_elem_id *dac1aifr_id;
	struct snd_ctl_elem_id *dac1_id;
	struct snd_ctl_elem_id *dac2_id;

	int fd = open("/dev/snd/controlC0", O_RDONLY);
	if(fd <=0)
		exit(-1);

	elem_list.offset = 0;
	elem_list.space  = 300;
	elements = ( struct snd_ctl_elem_id* ) malloc( elem_list.space * sizeof( struct snd_ctl_elem_id ) );
	elem_list.pids = elements;
	get_elem_list(fd,&elem_list);
	nelements = elem_list.used;

	pcm_playback_id = get_id("PCM Playback Sink");
	pcm_capture_id = get_id("PCM Capture Source");
	speaker_stereo_rx_id = get_id("speaker_stereo_rx");
	speaker_mono_tx_id = get_id("speaker_mono_tx");
	line1mix_id = get_id("LINEOUT1 Mixer Output Switch");
	line1outn_id = get_id("LINEOUT1N Switch");
	line1outp_id = get_id("LINEOUT1P Switch");
	line2mix_id = get_id("LINEOUT2 Mixer Output Switch");
	line2outn_id = get_id("LINEOUT2N Switch");
	line2outp_id = get_id("LINEOUT2P Switch");
	leftdacmix_id = get_id("Left Output Mixer DAC Switch");
	rightdacmix_id = get_id("Right Output Mixer DAC Switch");
	aif2adc_id = get_id("AIF2ADC HPF Switch");
	aif2adcvol_id = get_id("AIF2ADC Volume");
	aif2adcr_id = get_id("AIF2ADCR Source");
	aif2dacl_id = get_id("AIF2DAC2L Mixer Left Sidetone Switch");
	dac2vol_id = get_id("DAC2 Left Sidetone Volume");
	in1pgan_id = get_id("IN1L PGA IN1LN Switch");
	in1pgap_id = get_id("IN1L PGA IN1LP Switch");
	mixinl_id = get_id("MIXINL Output Record Volume");
	in1l_id = get_id("IN1L Switch");
	in1lvol_id = get_id("IN1L Volume");
	in1lzc_id = get_id("IN1L ZC Switch");
	mixinl1_id = get_id("MIXINL IN1L Switch");
	mixinlvol_id = get_id("MIXINL IN1L Volume");
	outvol_id = get_id("Output Volume");
	dac1aifl_id = get_id("DAC1L Mixer AIF1.1 Switch");
	dac1aifr_id = get_id("DAC1R Mixer AIF1.1 Switch");
	dac1_id = get_id("DAC1 Switch");
	dac2_id = get_id("DAC2 Switch");

//DSP Start
	write_elem(fd,pcm_playback_id, 0, 0, 1);
	write_elem(fd,pcm_capture_id, 0, 1, 1);
	write_elem(fd,speaker_stereo_rx_id, 1, 1, 1);
	write_elem(fd,speaker_mono_tx_id,1,1,1);

	write_elem(fd,pcm_playback_id, 0, 0, 0);
	write_elem(fd,pcm_capture_id, 0, 1, 0);
	write_elem(fd,speaker_stereo_rx_id, 0, 1, 0);
	write_elem(fd,speaker_mono_tx_id,0,1,0);

	write_elem(fd,pcm_playback_id, 0, 0, 1);
	write_elem(fd,pcm_capture_id, 0, 1, 1);
	write_elem(fd,speaker_stereo_rx_id, 1, 1, 1);
	write_elem(fd,speaker_mono_tx_id,1,1,1);
//End DSP Start

	write_elem(fd,line1mix_id,1,0,0);
	write_elem(fd,line1outn_id,1,0,0);
	write_elem(fd,line1outp_id,1,0,0);

	write_elem(fd,line2mix_id,1,0,0);
	write_elem(fd,line2outn_id,1,0,0);
	write_elem(fd,line2outp_id,1,0,0);

	write_elem(fd,leftdacmix_id,1,0,0);
	write_elem(fd,rightdacmix_id,1,0,0);
	write_elem(fd,aif2adc_id,1,1,0);
	write_elem(fd,aif2adcvol_id,0x64,0x64,0);
	write_elem(fd,aif2adcr_id,0,0,0);
	write_elem(fd,aif2dacl_id,1,0,0);
	write_elem(fd,dac2vol_id,0xC,0,0);

	write_elem(fd,in1pgan_id,1,0,0);
	write_elem(fd,in1pgap_id,1,0,0);
	write_elem(fd,mixinl_id,1,0,0);
	write_elem(fd,in1l_id,1,0,0);
	write_elem(fd,in1lvol_id,0x1B,0,0);
	write_elem(fd,in1lzc_id,0,0,0);

	write_elem(fd,mixinl1_id,1,0,0);
	write_elem(fd,mixinlvol_id,0,0,0);
	write_elem(fd,outvol_id,0x3B,0x3B,0);
	write_elem(fd,dac1aifl_id,1,0,0);
	write_elem(fd,dac1aifr_id,1,0,0);
	
	write_elem(fd,dac1_id,1,1,0);
	write_elem(fd,dac2_id,1,1,0);
	
	close(fd);

	printf("Version 6\n");
	return 0;
}
