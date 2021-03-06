//Data recorder mod with GUI showing light positions.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <survive.h>
#include <string.h>
#include <os_generic.h>
#include "src/survive_cal.h"
#include <DrawFunctions.h>

#include "src/survive_config.h"

struct SurviveContext * ctx;
int  quit = 0;

void HandleKey( int keycode, int bDown )
{
	if( !bDown ) return;

	if( keycode == 'O' || keycode == 'o' )
	{
		survive_send_magic(ctx,1,0,0);
	}
	if( keycode == 'F' || keycode == 'f' )
	{
		survive_send_magic(ctx,0,0,0);
	}
	if( keycode == 'Q' || keycode == 'q' )
	{
		quit = 1;
	}
}

void HandleButton( int x, int y, int button, int bDown )
{
}

void HandleMotion( int x, int y, int mask )
{
}

//int bufferpts[32*2*3][2];
int bufferpts[32*2*3][2];


char buffermts[32*128*3];
int buffertimeto[32*3][2];

void my_light_process( struct SurviveObject * so, int sensor_id, int acode, int timeinsweep, uint32_t timecode, uint32_t length  )
{
//	if( timeinsweep < 0 ) return;
	survive_default_light_process( so, sensor_id, acode, timeinsweep, timecode, length );

	if( sensor_id < 0 ) return;
	if( acode == -1 ) return;
//return;
	int jumpoffset = sensor_id;
	if( strcmp( so->codename, "WM0" ) == 0 ) jumpoffset += 32;
	else if( strcmp( so->codename, "WM1" ) == 0 ) jumpoffset += 64;


	if( acode == 0 || acode == 2 ) //data = 0
	{
		bufferpts[jumpoffset*2+0][0] = (timeinsweep-100000)/500;
		buffertimeto[jumpoffset][0] = 0;
	}
	if( acode == 1 || acode == 3 ) //data = 1
	{
		bufferpts[jumpoffset*2+1][0] = (timeinsweep-100000)/500;
		buffertimeto[jumpoffset][0] = 0;
	}


	if( acode == 4 || acode == 6 ) //data = 0
	{
		bufferpts[jumpoffset*2+0][1] = (timeinsweep-100000)/500;
		buffertimeto[jumpoffset][1] = 0;
	}
	if( acode == 5 || acode == 7 ) //data = 1
	{
		bufferpts[jumpoffset*2+1][1] = (timeinsweep-100000)/500;
		buffertimeto[jumpoffset][1] = 0;
	}
}

void my_imu_process( struct SurviveObject * so, int mask, FLT * accelgyro, uint32_t timecode, int id )
{
	survive_default_imu_process( so, mask, accelgyro, timecode, id );

	//if( so->codename[0] == 'H' )
	if( 0 )
	{
		printf( "I %s %d %f %f %f %f %f %f %d\n", so->codename, timecode, accelgyro[0], accelgyro[1], accelgyro[2], accelgyro[3], accelgyro[4], accelgyro[5], id );
	}
}


void my_angle_process( struct SurviveObject * so, int sensor_id, int acode, uint32_t timecode, FLT length, FLT angle )
{
	survive_default_angle_process( so, sensor_id, acode, timecode, length, angle );
}

char* sensor_name[32];

void * GuiThread( void * v )
{
	short screenx, screeny;
	CNFGBGColor = 0x000000;
	CNFGDialogColor = 0x444444;
	CNFGSetup( "Survive GUI Debug", 640, 480 );

	while(1)
	{
		CNFGHandleInput();
		CNFGClearFrame();
		CNFGColor( 0xFFFFFF );
		CNFGGetDimensions( &screenx, &screeny );

		int i,nn;
		for( i = 0; i < 32*3; i++ )
		{
			for( nn = 0; nn < 2; nn++ )
			{
				if( buffertimeto[i][nn] < 50 )
				{
					uint32_t color = i * 3231349;
					uint8_t r = 0xff;
					uint8_t g = 0x00;
					uint8_t b = 0xff;

					if (nn==0) b = 0; //lighthouse B
					if (nn==1) r = 0; //lighthouse C

//					r = (r * (5-buffertimeto[i][nn])) / 5 ;
//					g = (g * (5-buffertimeto[i][nn])) / 5 ;
//					b = (b * (5-buffertimeto[i][nn])) / 5 ;
					CNFGColor( (b<<16) | (g<<8) | r );
					CNFGTackRectangle( bufferpts[i*2+0][nn], bufferpts[i*2+1][nn], bufferpts[i*2+0][nn] + 5, bufferpts[i*2+1][nn] + 5 );
					CNFGPenX = bufferpts[i*2+0][nn]; CNFGPenY = bufferpts[i*2+1][nn];
					CNFGDrawText( buffermts, 2 );

					if (i<32) {
						CNFGPenX = bufferpts[i*2+0][nn]+5; CNFGPenY = bufferpts[i*2+1][nn]+5;
						CNFGDrawText( sensor_name[i], 2 );
					}
					buffertimeto[i][nn]++;
				}
			}
		}

		CNFGColor( 0xffffff );
		char caldesc[256];
		survive_cal_get_status( ctx, caldesc, sizeof( caldesc ) );
		CNFGPenX = 3;
		CNFGPenY = 3;
		CNFGDrawText( caldesc, 4 );


		CNFGSwapBuffers();
		OGUSleep( 10000 );
	}
}




int main()
{
	ctx = survive_init( 0 );

	uint8_t i =0;
	for (i=0;i<32;++i) {
		sensor_name[i] = malloc(3);
		sprintf(sensor_name[i],"%d",i);
	}

	survive_install_light_fn( ctx,  my_light_process );
	survive_install_imu_fn( ctx,  my_imu_process );
	survive_install_angle_fn( ctx, my_angle_process );

	survive_cal_install( ctx );

	OGCreateThread( GuiThread, 0 );
	

	if( !ctx )
	{
		fprintf( stderr, "Fatal. Could not start\n" );
		return 1;
	}

	while(survive_poll(ctx) == 0 && !quit)
	{
		//Do stuff.
	}

	survive_close( ctx );

	printf( "Returned\n" );
}

