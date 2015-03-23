#include <pebble.h>
#include "face.h"
	
int main(void) {
	show_face();
	CustomInit();
	app_event_loop();
	hide_face();
}