/// \file
/// \brief Handles emulation speed throttling using the SDL timing functions.

#include "Qt/sdl.h"
#include "Qt/throttle.h"

static const double Slowest = 0.015625; // 1/64x speed (around 1 fps on NTSC)
static const double Fastest = 32;       // 32x speed   (around 1920 fps on NTSC)
static const double Normal  = 1.0;      // 1x speed    (around 60 fps on NTSC)

static uint64 Lasttime, Nexttime;
static double desired_frametime = (1.0 / 60.099823);
static int InFrame = 0;
double g_fpsScale = Normal; // used by sdl.cpp
bool MaxSpeed = false;

/* LOGMUL = exp(log(2) / 3)
 *
 * This gives us a value such that if we do x*=LOGMUL three times,
 * then after that, x is twice the value it was before.
 *
 * This gives us three speed steps per order of magnitude.
 *
 */
#define LOGMUL 1.259921049894873

/**
 * Refreshes the FPS throttling variables.
 */
void
RefreshThrottleFPS()
{
   double hz;
	int32_t fps = FCEUI_GetDesiredFPS(); // Do >> 24 to get in Hz
	int32_t T;

   hz = ( ((double)fps) / 16777216.0 );

	desired_frametime = 1.0 / ( hz * g_fpsScale );

	T = (int32_t)( desired_frametime * 1000.0 );

	if ( T < 0 ) T = 1;

   //printf("FrameTime: %llu  %llu  %f  %lf \n", fps, fps >> 24, hz, desired_frametime );

	Lasttime=0;   
	Nexttime=0;
	InFrame=0;
}

/**
 * Perform FPS speed throttling by delaying until the next time slot.
 */
int
SpeedThrottle()
{
	if (g_fpsScale >= 32)
	{
		return 0; /* Done waiting */
	}
	uint64 time_left;
	uint64 cur_time;
    
	if (!Lasttime)
	{
		Lasttime = SDL_GetTicks();
	}
    
	if (!InFrame)
	{
		InFrame = 1;
		Nexttime = Lasttime + desired_frametime * 1000;
	}
    
	cur_time  = SDL_GetTicks();
	if(cur_time >= Nexttime)
		time_left = 0;
	else
		time_left = Nexttime - cur_time;
    
	if (time_left > 50)
	{
		time_left = 50;
		/* In order to keep input responsive, don't wait too long at once */
		/* 50 ms wait gives us a 20 Hz responsetime which is nice. */
	}
	else
	{
		InFrame = 0;
	}
    
	//fprintf(stderr, "attempting to sleep %Ld ms, frame complete=%s\n",
	//	time_left, InFrame?"no":"yes");

	if ( time_left > 0 )
	{
		SDL_Delay(time_left);
	}
    
	if (!InFrame)
	{
		Lasttime = SDL_GetTicks();
		return 0; /* Done waiting */
	}
	return 1; /* Must still wait some more */
}

/**
 * Set the emulation speed throttling to the next entry in the speed table.
 */
void IncreaseEmulationSpeed(void)
{
	g_fpsScale *= LOGMUL;
    
	if(g_fpsScale > Fastest) g_fpsScale = Fastest;

	RefreshThrottleFPS();
     
	FCEU_DispMessage("Emulation speed %.1f%%",0, g_fpsScale*100.0);
}

/**
 * Set the emulation speed throttling to the previous entry in the speed table.
 */
void DecreaseEmulationSpeed(void)
{
	g_fpsScale /= LOGMUL;
	if(g_fpsScale < Slowest)
		g_fpsScale = Slowest;

	RefreshThrottleFPS();

	FCEU_DispMessage("Emulation speed %.1f%%",0, g_fpsScale*100.0);
}

int CustomEmulationSpeed(int spdPercent)
{
   if ( spdPercent < 1 )
   {
      return -1;
   }
	g_fpsScale = ((double)spdPercent) / 100.0f;

	if (g_fpsScale < Slowest)
   {
	   g_fpsScale = Slowest;
   }
   else if (g_fpsScale > Fastest)
   {
	   g_fpsScale = Fastest;
   }

	RefreshThrottleFPS();

	FCEU_DispMessage("Emulation speed %.1f%%",0, g_fpsScale*100.0);

   return 0;
}


/**
 * Set the emulation speed throttling to a specific value.
 */
void
FCEUD_SetEmulationSpeed(int cmd)
{
	MaxSpeed = false;
    
	switch(cmd) {
	case EMUSPEED_SLOWEST:
		g_fpsScale = Slowest;
		break;
	case EMUSPEED_SLOWER:
		DecreaseEmulationSpeed();
		break;
	case EMUSPEED_NORMAL:
		g_fpsScale = Normal;
		break;
	case EMUSPEED_FASTER:
		IncreaseEmulationSpeed();
		break;
	case EMUSPEED_FASTEST:
		g_fpsScale = Fastest;
		MaxSpeed = true;
		break;
	default:
		return;
	}

	RefreshThrottleFPS();

	FCEU_DispMessage("Emulation speed %.1f%%",0, g_fpsScale*100.0);
}
