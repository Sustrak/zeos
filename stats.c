#include <stats.h>
#include <utils.h>

void init_stats(struct stats *s) {
  s->blocked_ticks = 0;
  s->elapsed_total_ticks = get_ticks();
  s->ready_ticks = 0;
  s->remaining_ticks = get_ticks();
  s->system_ticks = 0;
  s->total_trans = 0;
  s->user_ticks = 0;
}

void update_user_ticks(struct stats* st) {
  st->user_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
}

void update_sys_ticks(struct stats* st) {
  st->system_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
}

void update_ready_ticks(struct stats* st) {
  st->ready_ticks += get_ticks() - st->elapsed_total_ticks;
  st->elapsed_total_ticks = get_ticks();
}


