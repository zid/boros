#include "timer.h"
#include "rom.h"
#include "mem.h"
#include "cpu.h"
#include "lcd.h"
#include "sdl.h"

int main(void)
{
	int r;

	rom_load();
	lcd_init();
	mem_init();
	cpu_init();

	r = 0;

	while(1)
	{
		int now;

		if(!cpu_cycle())
			break;

		now = cpu_get_cycles();

		while(now != r)
		{
			int i;

			for(i = 0; i < 4; i++)
				if(!lcd_cycle())
					goto out;

			r++;
		}

		timer_cycle();

		r = now;
	}
out:
	return 0;
}
