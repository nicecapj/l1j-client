#ifndef __SDL_BUTTON_H_
#define __SDL_BUTTON_H_

#include <SDL.h>

#include "globals.h"
#include "sdl_widget.h"

sdl_graphic *make_sdl_graphic(int num, int x, int y, graphics_data *packs);
SDL_Rect *make_sdl_rect(int x, int y, int w, int h);

#include <SDL.h>

class sdl_button : public sdl_widget
{
	public:
		sdl_button(int num, int x, int y, graphics_data *packs, void (*fnctn)(void*, void*), void* ag, void* ag2);
		virtual ~sdl_button();
		
		virtual void cursor_on();
		virtual void cursor_off();		
	protected:
		void (*click_ptr)(void*, void*);
		void *arg1;
		void *arg2;
		
		void mouse_to(SDL_MouseMotionEvent *to);
		void mouse_from(SDL_MouseMotionEvent *from);
		void mouse_move(SDL_MouseMotionEvent *from, SDL_MouseMotionEvent *to);
		virtual void mouse_click(SDL_MouseButtonEvent *here);
};

#endif