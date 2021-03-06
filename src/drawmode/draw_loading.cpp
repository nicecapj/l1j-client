#include "draw_loading.h"

#include "globals.h"
#include "resources/prepared_graphics.h"
#include "sdl_user.h"
#include "widgets/sdl_animate_button.h"
#include "widgets/sdl_input_box.h"
#include "widgets/sdl_text_button.h"
#include "widgets/sprite.h"
#include "widgets/text_box.h"

load_ptr::load_ptr(draw_loading *bla, int val)
{
	ref = bla;
	which = val;
}

void load_ptr::go()
{
	ref->server_picked(which);
}

draw_loading::draw_loading(sdl_user *self)
	: sdl_drawmode(self)
{
	owner->game_music.change_music("sound/music0.mp3");
	load_progress = 0;
	load_amount = 0x249f0;	//not sure how this number is generated
		
	int index;
	SDL_Rect *rect;

	num_gfx = 2;
	gfx = new sdl_graphic*[num_gfx];
	gfx[0] = new sdl_graphic();
	gfx[0]->delay_load(811, 0, 0, GRAPH_PNG, owner);
	
	gfx[1] = new sdl_graphic();
	gfx[1]->delay_load(330, 241, 385, GRAPH_IMG, owner);

	num_servers = owner->get_config()->get_num_servers();
	
	num_widgets = num_servers + 1;
	widgets = new sdl_widget*[num_widgets];
	
	server_pick = -1;
	spick_mtx = SDL_CreateMutex();
	
	widget_key_focus = 1;
	for (int i = 0; i < num_servers; i++)
	{
		widgets[i+1] = new sdl_text_button(owner->get_config()->get_name(i), 276, 254+(15*(i+1)), owner, 
			(funcptr*)new load_ptr(this, i));
		widgets[i+1]->set_key_focus(true);
	}
	widgets[1]->cursor_on();
	widgets[0] = new text_box(257, 254, 150, 10*12, owner);
	
	//widgets[num_servers+1] = new sprite(320, 200, owner);//"2786-8.spr", owner);
	//widgets[num_servers+1] = new sprite(50, 50, "6258-0.spr", owner);
		//6256-173 nothing?
		//6256-181 nothing?
		//		
	
	((text_box*)widgets[0])->add_line("Please select a server:");
		
	update_load();
}

int draw_loading::get_server_pick()
{
	return server_pick;
}

bool draw_loading::quit_request()
{
	return true;
}

void draw_loading::add_text(char *bla)
{
	while (SDL_mutexP(draw_mtx) == -1) {};
	((text_box*)widgets[0])->add_line(bla);
	SDL_mutexV(draw_mtx);
}

void draw_loading::server_picked(int i)
{
	while (SDL_mutexP(spick_mtx) == -1) {};
	//hide buttons, add text to the textbox
	for (int ij = 1; ij <= num_servers; ij++)
	{
		widgets[ij]->hide(false);
		widgets[ij]->set_key_focus(false);
	}
	((text_box*)widgets[0])->add_line("Checking for updates");
	widgets[0]->cursor_on();
	server_pick = i;
	SDL_mutexV(spick_mtx);
}

void draw_loading::set_load_amount(int size)
{
	load_amount = size;
}

void draw_loading::update_load()
{
	float temp = (float)gfx[1]->getw() * ((float)load_progress / (float)load_amount);
	gfx[1]->setmw(temp);
}

void draw_loading::add_loaded(int size)
{
	load_progress += size;
	update_load();
}

void draw_loading::load_done()
{
	load_progress = load_amount;
	update_load();
	SDL_Delay(250);
	owner->change_drawmode(DRAWMODE_LOGIN);
}

draw_loading::~draw_loading()
{
	SDL_DestroyMutex(spick_mtx);
}

bool draw_loading::mouse_leave()
{
	return false;
}

void draw_loading::draw(SDL_Surface *display)
{
	SDL_FillRect(display, NULL, 0);
	sdl_drawmode::draw(display);
}