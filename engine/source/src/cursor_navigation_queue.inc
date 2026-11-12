/* Main is the only consumer; IRQ may append but cannot move head here.
 * Reject combined-direction masks and all action barriers. Twelve entries
 * leave four FIFO slots for input captured during abort/restore. Nothing
 * here bounds how long that tail takes. */
static uint8_t cursor_navigation_queue(const volatile uint8_t *queue,uint8_t head,uint8_t count){
 uint8_t i,key;
 if(count>12u)return 0;
 for(i=0;i<count;i++){
  key=queue[(head+i)&15u];
  if(key!=J_LEFT&&key!=J_RIGHT)return 0;
 }
 return 1;
}
