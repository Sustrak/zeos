


char buff[24];

int pid;


int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
  //runjp();
  //runjp_rank(6, 6);

  char c[20];
  int ret = read(0, &c, 4);
  write(1, itoc(ret), 1);
  ret = read(0, &c, 20);
  write(1, itoc(ret), 2);

  while(1) {
	
  }
}
