#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SIZE	(128*512) //128 sectors

extern "C" {
	bool nand_ReadSectors(sec_t sector, sec_t numSectors,void* buffer);
	bool nand_WriteSectors(sec_t sector, sec_t numSectors,void* buffer); //!!!
}

int menuTop = 5, statusTop = 18;
char dirname[15] = {0};

u32 sysid=0;
u32 ninfo=0;
u32 sizMB=0;
u32 System=0;
char nand_type[80]={0};
char nand_dump[80]={0};
char nand_rest[80]={0};
u8 *firmware_buffer; //raw dump and restore in first two options
u8 *fbuff; //sd firm files
u8 *xbuff; //xorpad
u8 *nbuff; //nand
u32 N3DS=2;
u32 O3DS=1;

/*
//---------------------------------------------------------------------------------
int saveToFile(const char *filename, u8 *buffer, size_t size) {
//---------------------------------------------------------------------------------
	FILE *f = fopen(filename, "wb");
	if (NULL==f) return -1;
	size_t written = fwrite(buffer, 1, size, f);
	fclose(f);
	if (written != size) return -2;
	return 0;
}
*/
struct menuItem {
	const char* name;
	fp function;
};

//---------------------------------------------------------------------------------
void clearStatus() {
//---------------------------------------------------------------------------------
	iprintf("\x1b[%d;0H\x1b[J\x1b[15;0H",statusTop); 
	iprintf("                                ");    //clean up after previous residents
	iprintf("                                ");
	iprintf("                                ");
	iprintf("                                ");
	iprintf("\x1b[%d;0H\x1b[J\x1b[15;0H",statusTop);
}

void chk() {
	
	nand_ReadSectors(0 , 1 , firmware_buffer);
	memcpy(&sysid, firmware_buffer + 0x100, 4);
	memcpy(&ninfo, firmware_buffer + 0x104, 4);
	
	if     (ninfo==0x00200000){strcpy(nand_type,"F0F1_O3DS.BIN"); System=O3DS; iprintf("OLD 3DS\n");} //old3ds
	else if(ninfo==0x00280000){strcpy(nand_type,"F0F1_N3DS.BIN"); System=N3DS; iprintf("NEW 3DS\n");} //new3ds
	else if(sysid!=0x4453434E){sizMB=0;   strcpy(nand_type,"");}              //dsi - lets do this in another app
	else                      {sizMB=0;   strcpy(nand_type,"");}              //not recognized, do nothing
	sprintf(nand_dump,"Dump    %s",nand_type);
	sprintf(nand_rest,"Restore %s",nand_type);
	
}

//---------------------------------------------------------------------------------
void backupNAND() {
//---------------------------------------------------------------------------------

	clearStatus();
	sizMB=8;

	if (!__dsimode) {
		iprintf("Not a 3ds!\n");
	} else {

		FILE *f = fopen(nand_type, "wb");

		if (NULL == f) {
			iprintf("failure creating %s\n", nand_type);
		} else {
			iprintf("Writing %s/%s\n\n", dirname, nand_type);
			size_t foffset=0x0B130000/0x200; //firm0 nand offset
			size_t i; 
			size_t sectors = 128;
			size_t blocks = (sizMB * 1024 * 1024) / (512);
			for (i=0; i < blocks; i+=128) {
				if(!nand_ReadSectors(i + foffset,sectors,firmware_buffer)) {
					iprintf("\nError reading FIRM!\n");
					break;
				}
				size_t written = fwrite(firmware_buffer, 1, 512 * sectors, f);
				
				if(written != 512 * sectors) {
					iprintf("\nError writing to SD!\n");
					break;
				}
				iprintf("%d of %d\r", i+128, blocks);
			}
			fclose(f);
		}
	}
	iprintf("\nDone!\r");

}

//---------------------------------------------------------------------------------
void restoreNAND() {
//---------------------------------------------------------------------------------

	clearStatus();
	sizMB=8;

	if (!__dsimode) {
		iprintf("Not a DSi or 3ds!\n");
	} else {
		
		iprintf("Sure? NAND restore is DANGEROUS!");
		iprintf("START + SELECT confirm\n");
		iprintf("B to exit\n");
		
		while(1){
		    scanKeys();
			int keys = keysHeld();
			if((keys & KEY_START) && (keys & KEY_SELECT))break;
			if(keys & KEY_B){
				clearStatus();
				return;
			}
			swiWaitForVBlank();
		}
		
		clearStatus();
	
		FILE *f = fopen(nand_type, "rb");

		if (NULL == f) {
			iprintf("failure creating %s\n", nand_type);
		} else {
			iprintf("Reading %s/%s\n\n", dirname, nand_type);
			size_t foffset=0x0B130000/0x200; //firm0 nand offset
			size_t i; 
			size_t sectors = 128;
			size_t blocks = (sizMB * 1024 * 1024) / (512);
			for (i=0; i < blocks; i+=128) {
				
				size_t read = fread(firmware_buffer, 1, 512 * sectors, f);
				
				if(read != 512 * sectors) {
					iprintf("\nError reading SD!\n");
					break;
				}
				
				if(!nand_WriteSectors(i + foffset,sectors,firmware_buffer)) {
					iprintf("\nError writing FIRM!\n");
					break;
				}
				
				iprintf("%d/%d DON'T poweroff!\r", i+128, blocks);
			}
			fclose(f);
		}
	}
	iprintf("\nDone!\r");
}

void xorbuff(u8 *in1, u8 *in2, u8 *out){
	
	for(int i=0; i < MAX_SIZE; i++){  //not concerned about the portability of a 6 liner
		out[i] = in1[i] ^ in2[i];
	}
	
}

void dgFIRM() {
//---------------------------------------------------------------------------------
	clearStatus();
	sizMB=4;
	
	u32 werror=0;
	u32 rerror=0;
	
	if (!__dsimode) {
		iprintf("Not a 3ds!\n");
	} else {
		
		iprintf("Sure?\nFIRM downgrade is DANGEROUS!\n");
		iprintf("START + SELECT confirm\n");
		iprintf("B to exit\n");
		
		while(1){
		    scanKeys();
			int keys = keysHeld();
			if((keys & KEY_START) && (keys & KEY_SELECT))break;
			if(keys & KEY_B){
				clearStatus();
				return;
			}
			swiWaitForVBlank();
		}
		
		clearStatus();
		
		char fname104[80]={0};
		char fname110[80]={0};
		
		if     (System==O3DS){
			sprintf(fname104,"firm104_%s.bin","OLD");
			sprintf(fname110,"firm110_%s.bin","OLD");
		}
		else if(System==N3DS){
		    sprintf(fname104,"firm104_%s.bin","NEW"); //firm104_OLD.bin firm110_OLD.bin
			sprintf(fname110,"firm110_%s.bin","NEW");
		}
		else {
			iprintf("System not recognized\n");
			return;
		}
	
		FILE *f104 = fopen(fname104,"rb");
		FILE *f110 = fopen(fname110,"rb");

		if (NULL == f110 || NULL == f104) {
			iprintf("failure opening sd files\n");
		} else {
			iprintf("Opening %s/%s\nand          */%s\n\n", dirname, fname104,fname110);
			size_t foffset=0x0B130000/0x200; //firm0 nand offset
			size_t i; 
			size_t sectors = 128;
			size_t blocks = (sizMB * 1024 * 1024) / (512);
			size_t rchk1=0;
			size_t rchk2=0;
			size_t rchk3=0;
			size_t wchk1=0;
			
			for (i=0; i < blocks; i+=128) { 
				
				rchk1 = fread(fbuff, 1, 512 * sectors, f110);        //get dec firm 11.0 on sd
				rchk2 = nand_ReadSectors(i + foffset,sectors,nbuff); //get enc firm 11.0 on nand

				xorbuff(fbuff,nbuff,xbuff);                          //xor the above two buffs to create xorpad buff
				
				rchk3 = fread(fbuff, 1, 512 * sectors, f104);        //get dec firm 10.4 on sd
				
				xorbuff(fbuff,xbuff,nbuff);                          //xor dec sd firm 10.4 and xorpad to create final encrypted image to write to nand
				
				if( (rchk1 != 512 * sectors) || (rchk3 != 512 * sectors) || (!rchk2) ) { rerror++; continue;} //assess read errors and skip writing if problem
				
				wchk1 = nand_WriteSectors(i + foffset,sectors,nbuff);//write to nand
				
				if(!wchk1) werror++;
				
				iprintf("%d/%d DON'T poweroff!\r", i+128, blocks);
		
			}
		
			fclose(f104);
			fclose(f110);
		}
	}
	iprintf("\nDone!\nreErr:%lu wrErr:%lu\r",rerror,werror);
}

u32 crc32b(u8 *message, u32 size) {  // http://www.hackersdelight.org/hdcodetxt/crc.c.txt
   u32 i, j;
   u32 byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   while (i < size) {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;
}

bool quitting = false;

//---------------------------------------------------------------------------------
void quit() {
//---------------------------------------------------------------------------------
	quitting = true;
}

struct menuItem mainMenu[] = {
	{ "Exit", quit },
	{ nand_dump , backupNAND},
	{ nand_rest , restoreNAND},
	{"Downgrade FIRM to 10.4",dgFIRM}
};

//---------------------------------------------------------------------------------
void showMenu(menuItem menu[], int count) {
//---------------------------------------------------------------------------------
	int i;
	for (i=0; i<count; i++ ) {
		iprintf("\x1b[%d;5H%s", i + menuTop, menu[i].name);
	}
}

//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
	//defaultExceptionHandler();

	consoleDemoInit();

	if (!fatInitDefault()) {
		printf("FAT init failed!\n");
	} else {

		iprintf("dgTool %s Beta\n",VERSION);

		firmware_buffer = (u8 *)memalign(32,MAX_SIZE);
		fbuff = (u8 *)memalign(32,MAX_SIZE);
	    nbuff = (u8 *)memalign(32,MAX_SIZE);
		xbuff = (u8 *)memalign(32,MAX_SIZE);

		readFirmware(0, firmware_buffer, 512);
		
		strcpy(dirname,"dgTool");

		mkdir(dirname, 0777);
		chdir(dirname);

		int count = sizeof(mainMenu) / sizeof(menuItem);
		
		chk();

		showMenu(mainMenu, count);
		
		int selected = 0;
		quitting = false;

		while(!quitting) {
				iprintf("\x1b[%d;3H]\x1b[23C[",selected + menuTop);
				swiWaitForVBlank();
				scanKeys();
				int keys = keysDownRepeat();
				iprintf("\x1b[%d;3H \x1b[23C ",selected + menuTop);
				if ( (keys & KEY_UP)) selected--;
				if (selected < 0)	selected = count - 1;
				if ( (keys & KEY_DOWN)) selected++;
				if (selected == count)	selected = 0;
				if ( keys & KEY_A ) mainMenu[selected].function();
		}
	}

	return 0;
}