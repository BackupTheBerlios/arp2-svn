/* Install private constructors and destructors pri MUST be -127<=pri<=127 */
#define ADD2INIT(a,pri) asm("	.section \".init\"");\
			asm("	.long " #a );\
			asm("	.long " #pri "+128");\
			asm("	.section \".text\"")
			
#define ADD2EXIT(a,pri) asm("	.section \".fini\"");\
			asm("	.long " #a );\
			asm("	.long " #pri "+128");\
			asm("	.section \".text\"")
