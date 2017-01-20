#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BLOCK_SIZE 1024
#define BLOCK_OFFSET 1024
//#define I_POINTER_W 4
#define INODE_SIZE 128
#define GDT_SIZE 32
char* fs = "disk-image"; 
int fs_id; 
char *file_system;
int st, sBuf32;
int image_fd, super_fd, bitmap_fd, group_fd, indirect_fd, dir_fd, inode_fd;
int num_group;
struct super_block *super;
struct cylinder_group *group;
uint64_t buf_64;
uint32_t buf_32;
uint16_t buf_16;
uint8_t buf_8; 
int **valid_inode_dir;
int valid_dir_count = 0;
int *valid_inodes;
int valid_inode_count = 0;

struct super_block{
  int magic_num, inode_num, block_num, frag_size, bpg,ipg,fpg, block_size,first_data_block;

};
struct cylinder_group{
  int CNTND_BLK, FR_BLOCK, FR_INODE, DIR, INODE_BITMAP, BLOCK_BITMAP, INODE_TABLE; 

};
void check_super_block(){
  if(super->magic_num != 0xEF53){
    fprintf(stderr, "Superblock - invalid magic :%x\n", super->magic_num);
    exit(1); 


  }
  if(super->block_num%super->bpg !=0){
    fprintf(stderr, "Superblock - %d blocks, %d blocks/group", super->block_num, super->bpg);
    exit(1); 

  }
  if(super->inode_num%super->ipg != 0){
    fprintf(stderr, "Superblock - %d inodes, %d inodes/group", super->inode_num, super->ipg);
    exit(1); 
  }
}
void check_cyclinder_group(struct cylinder_group grp){
  if(group->CNTND_BLK!=super->block_num){
    fprintf(stderr, "%d Blocks, super block says %d",group->CNTND_BLK, super->block_num);
    exit(1); 

  }
  //check free inode map starting
  
}
void error_msg(char *message){
  fprintf(stderr, "%s\n",message);
  exit(EXIT_FAILURE); 

}
void arguments(int argc, char **argv){
  if(argc !=2) {
    error_msg("Expected one argument\n");  
  }
  file_system = malloc((strlen(argv[1])+1)*sizeof(char));
  file_system = argv[1]; 	    
  

}
void initialize_image(){
  image_fd = open(file_system, O_RDONLY);
  super = malloc(sizeof(struct super_block));
  group = malloc(sizeof(struct cylinder_group));
  
}
void parse_super_group(){
  super_fd = creat("super.csv", S_IRWXU);
  //capture the magic number
  pread(image_fd, &buf_16,2,BLOCK_OFFSET+56);
  super->magic_num = buf_16;
  dprintf(super_fd, "%x,",super->magic_num);

  //capture the inode count
  pread(image_fd, &buf_32,4,BLOCK_OFFSET+0);
  super->inode_num = buf_32;
  dprintf(super_fd, "%d,",super->inode_num);
  //capture number of blocks
  pread(image_fd, &buf_32,4,BLOCK_OFFSET+4);
  super->block_num = buf_32;
  dprintf(super_fd, "%d,",super->block_num);
  //capture block size 
  pread(image_fd, &buf_32,4,BLOCK_OFFSET+24);
  super->block_size = 1024<<buf_32;
  dprintf(super_fd, "%d,",super->block_size);
  //capture fragment size 
  pread(image_fd, &sBuf32,4,BLOCK_OFFSET+28);
  if(sBuf32>0){
    super->frag_size = 1024<<sBuf32;
  }
  else {
    super->frag_size = 1024>> -sBuf32; 
  }
  dprintf(super_fd, "%d,",super->frag_size);

  //capture blocks per group
  pread(image_fd, &buf_32,4,BLOCK_OFFSET+32);
  super->bpg = buf_32;
  dprintf(super_fd, "%d,",super->bpg);

  //capture inodes per group
  pread(image_fd, &buf_32,4,BLOCK_OFFSET+40);
  super->ipg = buf_32;
  dprintf(super_fd, "%d,",super->ipg);

  //capture fragments per group
  pread(image_fd, &buf_32,4,BLOCK_OFFSET+36);
  super->fpg = buf_32;
  dprintf(super_fd, "%d,",super->fpg);

  //capture first data block
  pread(image_fd, &buf_32,4,BLOCK_OFFSET+20);
  super->first_data_block = buf_32;
  dprintf(super_fd, "%x,",super->first_data_block);
  int mask = 1;
  mask = mask & super->block_size;
  
  close(super_fd); 
}
void parse_cylinder_group(){
  group_fd = creat("group.csv",S_IRWXU);

  num_group =(int) ceil((double)super->block_num/super->bpg);
  double excess = num_group - ((double)super->block_num/super->bpg);

  group = malloc(num_group*sizeof(struct cylinder_group));

  int i;
  for(i = 0; i < num_group; i++){
    //
    if(i!=num_group -1 || excess == 0){
      group[i].CNTND_BLK = super->bpg;
      dprintf(group_fd, "%d,",group[i].CNTND_BLK);
    }
    else {
      group[i].CNTND_BLK = super->bpg*excess;
      dprintf(group_fd, "%d,",group[i].CNTND_BLK);
    }
    //capture number of free blocks
    pread(image_fd, &buf_16, 2,BLOCK_OFFSET + BLOCK_SIZE + (i*GDT_SIZE)+12);
    group[i].FR_BLOCK = buf_16;
    dprintf(group_fd, "%d,",group[i].FR_BLOCK);

    //capture number of free inodes
    pread(image_fd, &buf_16, 2,BLOCK_OFFSET + BLOCK_SIZE + (i*GDT_SIZE)+14);
    group[i].FR_INODE = buf_16;
    dprintf(group_fd, "%d,",group[i].FR_INODE);

    //capture number od directories
    pread(image_fd, &buf_16, 2,BLOCK_OFFSET + BLOCK_SIZE + (i*GDT_SIZE)+16);
    group[i].DIR = buf_16;
    dprintf(group_fd, "%d,",group[i].DIR);

    //capture inode bitmap block
    pread(image_fd, &buf_32, 4,BLOCK_OFFSET + BLOCK_SIZE + (i*GDT_SIZE)+4);
    group[i].INODE_BITMAP = buf_32;
    dprintf(group_fd, "%x,",group[i].INODE_BITMAP);

    //capture block bitmap
    pread(image_fd, &buf_32, 4,BLOCK_OFFSET + BLOCK_SIZE + (i*GDT_SIZE)+0);
    group[i].BLOCK_BITMAP = buf_32;
    dprintf(group_fd, "%x,",group[i].BLOCK_BITMAP);

    //capture Inode Table
    pread(image_fd, &buf_32, 4,BLOCK_OFFSET + BLOCK_SIZE + (i*GDT_SIZE)+8);
    group[i].INODE_TABLE = buf_32;
    dprintf(group_fd, "%x,",group[i].INODE_TABLE);
    dprintf(group_fd, "\n");
    /*if(group[i].CNTND_BLK!=super->block_num){
      printf( "Group %d: %d blocks, superblock syas %d\n", i,group[i].CNTND_BLK, super->block_num); 

      }*/
    
  }
  close(group_fd);

}
void parse_bitmap(){
  bitmap_fd = creat("bitmap.csv",S_IRWXU);
  int i;
  for(i = 0; i < num_group; i++){
    
    int j;
    for(j = 0; j < super->block_size; j++) {//dont confuse blcok size with block num
      //read byte
      pread(image_fd, &buf_8, 1, group[i].BLOCK_BITMAP*super->block_size+j);
      int8_t mask = 1;
      
      //look at byte
      int k;
      for(k = 1; k <=8; k++){
	int val = buf_8 & mask;
	if(val == 0){
	  dprintf(group_fd, "%x,", group[i].BLOCK_BITMAP);
	  dprintf(group_fd, "%d", j*8+k+ i*super->bpg);
	  dprintf(group_fd, "\n");

	}
	mask = mask <<1; 
    }
    }
    //check each byte in node bitmap
    for(j = 0; j < super->block_size; j++){
      //read a byte
      pread(image_fd, &buf_8, 1, group[i].INODE_BITMAP*super->block_size+j);
      int8_t mask = 1;
      //analyze the byte
      int k;
      for(k = 1; k <=8; k++){
	int val = buf_8 & mask;
	if(val ==0){
	  dprintf(group_fd, "%x,", group[i].INODE_BITMAP);
	  dprintf(group_fd, "%d", j*8+k+i*super->ipg);
	  dprintf(group_fd, "\n");
	  

	}
	mask = mask <<1; 
    }

  }
  }
  close(bitmap_fd); 

}
void parse_inode(){
  inode_fd = creat("inode.csv", S_IRWXU);
  //allocate the data structure
  valid_inode_dir = malloc(super->inode_num*sizeof(int*));

  int i;
  for(i = 0; i < super->inode_num; i++){
    valid_inode_dir[i] = malloc(2*sizeof(int));
    

  }
  valid_inodes = malloc(super->inode_num*sizeof(int));

  for(i = 0; i < num_group; i++){
    //check each byte

    int j ;
    for(j = 0; j < super->block_size; j++){
      //read the byte
      pread(image_fd, &buf_8, 1, (group[i].INODE_BITMAP*super->block_size)+j);
      int8_t mask = 1;
      //analuze the byte
      int k;
      for(k = 1; k <=8; k++){
	int val = buf_8 & mask;
	if(val!= 0 && (j*8+k)<=super->ipg){
	  //get inode num
	  int inode_num = j*8+k+(i*super->ipg);
	  dprintf(inode_fd, "%d,", inode_num);
	  //get the file type
	  pread(image_fd, &buf_16, 2, group[i].INODE_TABLE*super->block_size+(j*8+k-1)*INODE_SIZE+0);
	  valid_inodes[valid_inode_count] = group[i].INODE_TABLE*super->block_size+(j*8+k-1)*INODE_SIZE;
	  valid_inode_count++;
	  //if it is a reg file
	  if(buf_16 & 0x8000){
	    dprintf(inode_fd, "f,"); 
	  }
	  //symbolic files
	  else if (buf_16 & 0xA000){
	    dprintf(inode_fd, "s,"); 
	  }

	  //check for directories
	  else if(buf_16 & 0x4000){
	    valid_inode_dir[valid_dir_count][0] = group[i].INODE_TABLE*super->block_size+(j*8+k-1)*INODE_SIZE;
	    valid_inode_dir[valid_dir_count][1] = inode_num;
	    valid_dir_count++;
	    dprintf(inode_fd, "d,");

	  }
	  else {
	    dprintf(inode_fd, "?,"); 

	  }
	  //capture mode
	  dprintf(inode_fd, "%o,",buf_16);
	  
	  //capture owner
	  uint32_t uid;
	  pread(image_fd, &buf_32, 2, group[i].INODE_TABLE*super->block_size+ (j*8+k-1)*INODE_SIZE +2);
	  uid = buf_32;
	  pread(image_fd, &buf_32, 2, group[i].INODE_TABLE*super->block_size+(j*8+k-1)*INODE_SIZE+116+4);
	  uid = uid | (buf_32 <<16);
	  dprintf(inode_fd, "%d,", uid);

	  //capture group
	  uint32_t gid;
	  pread(image_fd, &buf_32, 2, group[i].INODE_TABLE*super->block_size+ (j*8+k-1)*INODE_SIZE +24);
	  gid = buf_32;
	  pread(image_fd, &buf_32, 2, group[i].INODE_TABLE*super->block_size+(j*8+k-1)*INODE_SIZE+116+6);
	  gid = gid | (buf_32 <<16);
	  dprintf(inode_fd, "%d,", gid);

	  //link count
	  pread(image_fd, &buf_32, 2, group[i].INODE_TABLE*super->block_size + (j*8+k-1)*INODE_SIZE+26);
	  dprintf(inode_fd, "%d,",buf_16);

	  //creation time
	  pread(image_fd, &buf_32, 4, group[i].INODE_TABLE*super->block_size + (j*8+k-1)*INODE_SIZE+12);
	  dprintf(inode_fd, "%d,",buf_32);

	  //modification time
	  pread(image_fd, &buf_32, 4, group[i].INODE_TABLE*super->block_size + (j*8+k-1)*INODE_SIZE+16);
	  dprintf(inode_fd, "%x,",buf_32);

	  //file size
	  uint64_t size;
	  pread(image_fd, &buf_64, 4, group[i].INODE_TABLE*super->block_size+ (j*8+k-1)*INODE_SIZE +4);
	  size = buf_64;
	  pread(image_fd, &buf_64, 4, group[i].INODE_TABLE*super->block_size+(j*8+k-1)*INODE_SIZE+108);
	  size = size | (buf_64 <<32);
	  dprintf(inode_fd, "%ld,", size);

	  //number of blocks
	  pread(image_fd, &buf_32, 4, group[i].INODE_TABLE*super->block_size + (j*8+k-1)*INODE_SIZE+28);
	  dprintf(inode_fd, "%d,",buf_32/(super->block_size/512));

	  //block pinters
	  int s;
	  for(s = 0; s < 15; s++){
	    pread(image_fd, &buf_32, 4, group[i].INODE_TABLE*super->block_size+(j*8+k-1)*INODE_SIZE+40+(s*4));
	    if(s!=14){
	      dprintf(inode_fd, "%x,", buf_32);
	      
	    }
	    else {
	      dprintf(inode_fd, "%x",buf_32);

	    }
	  }
	  dprintf(inode_fd, "\n");

	  buf_16 = 0;
	  buf_32 = 0;
	  buf_64 = 0; 
     
	}
	mask = mask <<1;

      }


    }
  }
  close(inode_fd);

}
void parse_directory(){
  dir_fd = creat("directory.csv", S_IRWXU);
  //dprintf(dir_fd, "hello son\n");
  //loop through dir inodes
  //printf("%d\n", valid_dir_count); 
  int i;
  for(i = 0; i < valid_dir_count; i++){
    int entry = 0;
    int j;
    for(j = 0; j < 12; j++){
      //printf("cp 1\n"); 
      pread(image_fd, &buf_32, 4, (valid_inode_dir[i][0]+40+(j*4)));
      //printf("%d\n", buf_32); 
      uint32_t d_offset = buf_32;
      //printf("%d\n",d_offset); 
      if(d_offset != 0){
	//printf("d_offset!=0\n");
	int current_offset = super->block_size*d_offset;
	while(current_offset < super->block_size*d_offset+super->block_size){
	  //name length
	  //printf("checkpoint 1\n");
	  pread(image_fd, &buf_8, 1, current_offset+6);
	  int name_length = buf_8;
	  

	  //inode number
	  pread(image_fd, &buf_32, 4, current_offset);
	  int inode_number = buf_32;

	  //entry length
	  pread(image_fd, &buf_16, 2, current_offset+4);
	  int entry_length = buf_16;

	  //if inode num = 0;
	  if(inode_number ==0){
	    current_offset += entry_length;
	    entry++;
	    continue; 

	  }
	  //capture parent inode number
	  //printf("checkpoint!\n"); 
	  dprintf(dir_fd, "%d,", valid_inode_dir[i][1]);
	  
	  //entry number
	  dprintf(dir_fd, "%d,", entry);
	  entry++;
	  
	  //entry length
	  dprintf(dir_fd, "%d,", entry_length);

	  //name length
	  dprintf(dir_fd, "%d,", name_length);

	  //inode number
	  dprintf(dir_fd, "%d,", inode_number);

	  //gatehr name
	  char c_buf;
	  dprintf(dir_fd, "\"");
	  int k;
	  for(k = 0; k < name_length;k++){
	    pread(image_fd, &c_buf, 1, current_offset+8+k);
	    dprintf(dir_fd, "%c", c_buf); 
	  }
	  dprintf(dir_fd, "\"\n");
	  current_offset += entry_length; 

	  


	}//end while
	
      }//end if


    }
    //indirect blocks
    pread(image_fd, &buf_32, 4, (valid_inode_dir[i][0]+40+(12*4)));
      uint32_t d_offset = buf_32;
      if(d_offset!=0){
	j = 0;
	for(j = 0; j < super->block_size/4; j++){
	  
	  int current_offset = super->block_size*d_offset+j*4;
	  pread(image_fd, &buf_32, 4, current_offset);
	  int block_num_2 = buf_32;
	  if(block_num_2 != 0){
	    current_offset = block_num_2*super->block_size; 
	    while(current_offset < super->block_size*d_offset+super->block_size){
	      //name length
	      pread(image_fd, &buf_8, 1, current_offset+6);
	      int name_length = buf_8;
	      
	      
	      //inode number
	      pread(image_fd, &buf_32, 4, current_offset);
	      int inode_number = buf_32;

	      //entry length
	      pread(image_fd, &buf_16, 2, current_offset+4);
	      int entry_length = buf_16;

	      //if inode num = 0;
	      if(inode_number ==0){
		current_offset += entry_length;
		entry++;
		continue; 

	      }
	      //capture parent inode number

	      dprintf(dir_fd, "%d,", valid_inode_dir[i][1]);

	      //entry number
	      dprintf(dir_fd, "%d,", entry);
	      entry++;

	      //entry length
	      dprintf(dir_fd, "%d,", entry_length);

	      //name length
	      dprintf(dir_fd, "%d,", name_length);

	      //inode number
	      dprintf(dir_fd, "%d,", inode_number);

	      //gatehr name
	      char c_buf;
	      dprintf(dir_fd, "\"");
	      int k;
	      for(k = 0; k < name_length;k++){
		pread(image_fd, &c_buf, 1, current_offset+8+k);
		dprintf(dir_fd, "%c", c_buf); 
	      }
	      dprintf(dir_fd, "\"\n");
	      current_offset += entry_length; 

	  


	    }//end while
	  }
	}
	
      }
      //double indirect blocks
      pread(image_fd, &buf_32, 4, (valid_inode_dir[i][0]+40+(13*4)));
      d_offset = buf_32;
      if(d_offset!=0){
	j = 0;
	for(j = 0; j < super->block_size/4; j++){
	  int current_offset = super->block_size*d_offset+ j*4;
	  pread(image_fd, &buf_32, 4, current_offset);
	  int block_num_2 = buf_32;
	  if(block_num_2!=0){
	    int k;
	    for(k = 0; k < super->block_size/4; k++){
	      pread(image_fd, &buf_32, 4, block_num_2*super->block_size+ k*4);
	      int block_num_3 = buf_32;
	      if(block_num_3!=0){
		current_offset =block_num_3*super->block_size;
	  while(current_offset < block_num_3*super->block_size+super->block_size){
	    
	    //name length
	    pread(image_fd, &buf_8, 1, current_offset+6);
	    int name_length = buf_8;
	  

	    //inode number
	    pread(image_fd, &buf_32, 4, current_offset);
	    int inode_number = buf_32;

	    //entry length
	    pread(image_fd, &buf_16, 2, current_offset+4);
	    int entry_length = buf_16;

	    //if inode num = 0;
	    if(inode_number ==0){
	      current_offset += entry_length;
	      entry++;
	      continue; 

	    }
	    //capture parent inode number

	    dprintf(dir_fd, "%d,", valid_inode_dir[i][1]);

	    //entry number
	    dprintf(dir_fd, "%d,", entry);
	    entry++;

	    //entry length
	    dprintf(dir_fd, "%d,", entry_length);

	    //name length
	    dprintf(dir_fd, "%d,", name_length);

	    //inode number
	    dprintf(dir_fd, "%d,", inode_number);

	    //gatehr name
	    char c_buf;
	    dprintf(dir_fd, "\"");
	    int k;
	    for(k = 0; k < name_length;k++){
	      pread(image_fd, &c_buf, 1, current_offset+8+k);
	      dprintf(dir_fd, "%c", c_buf); 
	    }
	    dprintf(dir_fd, "\"\n");
	    current_offset += entry_length; 

	  


	  }//end while
	      }
	    }
	  }
	}//end if


    }

      //triple indirect blocks
      pread(image_fd, &buf_32, 4, (valid_inode_dir[i][0]+40+(14*4)));
      d_offset = buf_32;
      if(d_offset!=0){
	j = 0;
	for(j = 0; j < super->block_size/4;j++){
	  int current_offset = super->block_size*d_offset+j*4;
	  pread(image_fd, &buf_32, 4, current_offset);
	  int block_num_2 = buf_32;
	  if(block_num_2!=0){
	    int k;
	    for(k = 0; k < super->block_size/4; k++){
	      pread(image_fd, &buf_32, 4, block_num_2*super->block_size + k*4);
	      int block_num_3 = buf_32;
	      if(block_num_3!=0){
		int s;
		for(s = 0; s < super->block_size/4; s++){
		  pread(image_fd, &buf_32, 4,block_num_3*super->block_size+(s*4));
		  int block_num_4 = buf_32;
		  if(block_num_4!=0){
		    current_offset = block_num_4*super->block_size;
		    
		    
		    while(current_offset < block_num_4*super->block_size+super->block_size){
		      //name length
		      pread(image_fd, &buf_8, 1, current_offset+6);
		      int name_length = buf_8;
	  

		      //inode number
		      pread(image_fd, &buf_32, 4, current_offset);
		      int inode_number = buf_32;

		      //entry length
		      pread(image_fd, &buf_16, 2, current_offset+4);
		      int entry_length = buf_16;

		      //if inode num = 0;
		      if(inode_number ==0){
			current_offset += entry_length;
			entry++;
			continue; 

		      }
		      //capture parent inode number

		      dprintf(dir_fd, "%d,", valid_inode_dir[i][1]);

		      //entry number
		      dprintf(dir_fd, "%d,", entry);
		      entry++;

		      //entry length
		      dprintf(dir_fd, "%d,", entry_length);

		      //name length
		      dprintf(dir_fd, "%d,", name_length);

		      //inode number
		      dprintf(dir_fd, "%d,", inode_number);

		      //gatehr name
		      char c_buf;
		      dprintf(dir_fd, "\"");
		      int k;
		      for(k = 0; k < name_length;k++){
			pread(image_fd, &c_buf, 1, current_offset+8+k);
			dprintf(dir_fd, "%c", c_buf); 
		      }
		      dprintf(dir_fd, "\"\n");
		      current_offset += entry_length; 

	  


		    }//end while
	
		  }//end if


		}

	      }
	    }
	  }
	}
      }
  }
  close(dir_fd); 
}
void parse_indirect(){
  indirect_fd = creat("indirect.csv", S_IRWXU);
  int entry_num = 0;

  //valid nodes
  int i;

  for(i = 0; i < valid_inode_count; i++){
    //single indirect
    entry_num =0;
    pread(image_fd, &buf_32, 4, valid_inodes[i]+40+(12*4));
    int block_num = buf_32;
    int j;
    for(j = 0; j < super->block_size/4; j++){
      pread(image_fd, &buf_32, 4, block_num*super->block_size+j*4);
      int block_num_2 = buf_32;
      if(block_num_2!=0){
	dprintf(indirect_fd, "%x,", block_num);

	dprintf(indirect_fd, "%d,",entry_num);
	entry_num++;

	dprintf(indirect_fd, "%x\n", block_num_2);
	

      }

    }//close j for loop

    //double indirect
    entry_num =0;
    pread(image_fd, &buf_32, 4, valid_inodes[i]+40+(13*4));
    block_num = buf_32;
    
    for(j = 0; j < super->block_size/4; j++){
      pread(image_fd, &buf_32, 4, block_num*super->block_size+j*4);
      int block_num_2 = buf_32;
      if(block_num_2!=0){
	dprintf(indirect_fd, "%x,", block_num);

	dprintf(indirect_fd, "%d,",entry_num);
	entry_num++;

	dprintf(indirect_fd, "%x\n", block_num_2);
	

      }

    }//close j for loop

    entry_num =0;
    //pread(image_fd, &buf_32, 4, valid_inodes[i]+40+(12*4));
    //int block_num = buf_32;
    //int j;
    for(j = 0; j < super->block_size/4; j++){
      pread(image_fd, &buf_32, 4, block_num*super->block_size+j*4);
      int block_num_2 = buf_32;
      if(block_num_2!=0){
	entry_num = 0;
	int k;
	for(j = 0; k < super->block_size/4; k++){
	  pread(image_fd, &buf_32, 4, block_num_2*super->block_size+k*4);
	  int block_num_3 = buf_32;
	  if(block_num_3!=0){
	    dprintf(indirect_fd, "%x,", block_num);

	    dprintf(indirect_fd, "%d,",entry_num);
	    entry_num++;

	    dprintf(indirect_fd, "%x\n", block_num_2);
	  }
	}
      }

    }//close j for loop
    //triple indirects

    entry_num = 0;
    pread(image_fd, &buf_32, 4, valid_inodes[i]+40+14*4);
    block_num = buf_32;
    for(j = 0; j < super->block_size/4; j++){
      pread(image_fd, &buf_32, 4, block_num*super->block_size+j*4);
      int block_num_2 = buf_32;
      if(block_num_2!=0){
	dprintf(indirect_fd,"%x,", block_num);
	
	dprintf(indirect_fd,"%d,", entry_num);
	entry_num++;

	dprintf(indirect_fd, "%x\n", block_num_2);
      }
    }

    entry_num = 0;
    //pread(image_fd, &buf_32, 4, valid_inodes[i]+40+14*4);
    //block_num = buf_32;
    for(j = 0; j < super->block_size/4; j++){
      pread(image_fd, &buf_32, 4, block_num*super->block_size+j*4);
      int block_num_2 = buf_32;
      if(block_num_2!=0){
	entry_num = 0;
	int k;
	for(k = 0; k < super->block_size/4; k++){
	  pread(image_fd, &buf_32, 4, block_num_2*super->block_size +4*k);
	  int block_num_3 = buf_32;
	  if(block_num_3!=0){
	    dprintf(indirect_fd,"%x,", block_num_2);
	
	    dprintf(indirect_fd,"%d,", entry_num);
	    entry_num++;

	    dprintf(indirect_fd, "%x\n", block_num_3);
	  }
	}
	}
    }
    entry_num = 0;
    for(j = 0; j < super->block_size/4; j++){
      pread(image_fd, &buf_32, 4, block_num*super->block_size+j*4);
      int block_num_2 = buf_32;
      if(block_num_2!=0){
	entry_num = 0;
	int k;
	for(k = 0; k < super->block_size/4; k++){
	  pread(image_fd, &buf_32, 4, block_num_2*super->block_size+ k*4);
	  int block_num_3 = buf_32;
	  if(block_num_3!=0){
	    entry_num = 0;
	    int s;
	    for(s = 0; s < super->block_size/4; s++){
	      pread(image_fd, &buf_32, 4, block_num_3*super->block_size+s*4);
	      int block_num_4 = buf_32;
	      if(block_num_4!=0){
		dprintf(indirect_fd, "%x,", block_num_3);
		dprintf(indirect_fd, "%d,",entry_num);
		entry_num++;

		dprintf(indirect_fd, "%x\n", block_num_4);
	      }
	    }
	  }
	}
      }
    }


  }//close i for looop 
  
  close(indirect_fd);

}
int main(int argc, char **argv){
  arguments(argc, argv);
  initialize_image();
  
  parse_super_group();
  check_super_block(); // check if super block parameteres are correct
  parse_cylinder_group();
  parse_bitmap();
  parse_inode();
  parse_directory();
  parse_indirect();
  close(image_fd);
  exit(EXIT_SUCCESS);
  
  return 0; 
}
