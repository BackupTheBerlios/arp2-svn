
#define LP0NR(off, name, dummy, base) off: void name() ()
#define LP0(off, ret, name, dummy, base) off: ret name() ()
#define LP1(off, ret, name, t1, n1, r1, dummy, base) off: ret name(t1 n1) (r1)
#define LP2(off, ret, name, t1, n1, r1, t2, n2, r2, dummy, base) off: ret name(t1 n1, t2 n2) (r1,r2)
#define LP3(off, ret, name, t1, n1, r1, t2, n2, r2, t3, n3, r3, dummy, base) off: ret name(t1 n1, t2 n2, t3 n3) (r1,r2,r3)
