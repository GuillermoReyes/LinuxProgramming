#!/usr/bin/python

import sys, os, csv
#create an object for each inode
class Inode(object):
    def __init__(self, number, link_count):
        self._number = number
        self._referenced_by = []
        self._link_count = link_count
        self._ptr_list = []
        
        return 

#create an object for each block
class Block(object):
    def __init__(self, number):
        self._number = number
        self._referenced_by = []
        return 
#create class to parse csv file

class csv_parser(object):
    def __init__(self):
        self._block_count = 0
        self._inode_count = 0
        self._block_size = 0
        self._bpg = 0
        self._ipg = 0

        self._inode_dict = dict()
        self._block_dict = dict()

        self._indirect_dict = dict()
        self._directory_dict = dict()
        self._inode_bitmap_blocks = []
        self._block_bitmap_blocks = []
        self._inode_free_list = []
        self._block_free_list = []
        
        self._unallocated_inodes = dict()
        
        self._incorrect_entries = []
        self._invalid_blocks = []
        
        return
    def read_super(self, filename = "super.csv"):
        with open(filename, 'r') as csv_file:
            reader = csv.reader(csv_file, delimiter=',', quotechar='|')
            for line in reader:
                self._inode_count = int(line[1])
                self._block_count = int(line[2])
                self._block_size = int(line[3])
                self._bpg = int(line[5])
                self._ipg = int(line[6])
        return 

    def read_group(self, file_name = "group.csv"):
        with open(file_name, 'r') as csv_file:
            reader = csv.reader(csv_file, delimiter=',',quotechar='|')
            for line in reader:
                self._inode_bitmap_blocks.append(int(line[4],16))
                self._block_bitmap_blocks.append(int(line[5],16))
                
                inode_bitmap_block = Block(int(line[4],16))
                self._block_dict[int(line[4],16)] = inode_bitmap_block
                block_bitmap_block = Block(int(line[5],16))
                self._block_dict[int(line[5],16)] = block_bitmap_block
        return


    def read_bitmap(self, file_name = "bitmap.csv"):
        with open(file_name, 'r') as csv_file:
            reader = csv.reader(csv_file, delimiter=',',quotechar='|')
            for line in reader:
                bitmap_block_number = int(line[0], 16)
                if bitmap_block_number in self._inode_bitmap_blocks:
                    self._inode_free_list.append(int(line[1]))
                elif bitmap_block_number in self._block_bitmap_blocks:
                    self._block_free_list.append(int(line[1]))
        return 

    def read_indirect(self, file_name = "indirect.csv"):
        with open(file_name, 'r') as csv_file:
            reader = csv.reader(csv_file, delimiter=',',quotechar='|')
            for line in reader:
                containing_block = int(line[0], 16)
                entry_num = int(line[1])
                pointer_value = int(line[2],16)
                if containing_block in self._indirect_dict:
                    self._indirect_dict[containing_block].append((entry_num,pointer_value))
                else:
                    self._indirect_dict[containing_block] = [(entry_num, pointer_value)]
        return 
    def read_directory(self, file_name="directory.csv"):
        with open(file_name, 'r') as csv_file:
            reader = csv.reader(csv_file,delimiter=',',quotechar='|')
            for line in reader:
                child_inode_num = int(line[4])
                parent_inode_num = int(line[0])
                entry_num = int(line[1])
                entry_name = line[5]
                if child_inode_num != parent_inode_num or parent_inode_num == 2:
                    self._directory_dict[child_inode_num] = parent_inode_num
                if child_inode_num in self._inode_dict:
                    self._inode_dict[child_inode_num]._referenced_by.append((parent_inode_num,entry_num))
                else:
                    if child_inode_num in self._unallocated_inodes:
                        self._unallocated_inodes[child_inode_num].append((parent_inode_num, entry_num))
                    else:
                        self._unallocated_inodes[child_inode_num]=[(parent_inode_num, entry_num)]
                if entry_num == 0:
                    if child_inode_num != parent_inode_num:
                        self._incorrect_entries.append((parent_inode_num, entry_name, child_inode_num, parent_inode_num))
                elif entry_num == 1:
                    if parent_inode_num not in self._directory_dict or child_inode_num != self._directory_dict[parent_inode_num]:
                        self._incorrect_entries.append((parent_inode_num, entry_name, child_inode_num, self._directory_dict[parent_inode_num]))
        return 
    def read_inode(self,file_name = "inode.csv"):
        with open(file_name, 'r') as csv_file:
            reader = csv.reader(csv_file, delimiter=',',quotechar='|')
            for line in reader:
                number_blocks = int(line[10])
                inode_number = int(line[0])
                link_count = int(line[5])
                self._inode_dict[inode_number] = Inode(inode_number, link_count)
                
                #data blocks
                upper_bound = min(12,number_blocks)+11
                for i in range(11, upper_bound):
                    block_number = int(line[i], 16)
                    self.increment_block_referenced_by(block_number,inode_number,0,i-11)
                #indirect data blocks
                remaining_blocks = number_blocks - 12
                if remaining_blocks > 0:
                    block_number = int(line[23],16)
                    if block_number == 0 or block_number > self._block_count:
                        self._invalid_blocks.append((block_number, inode_number,0,12))
                    else:
                        count = self.scan_indirect_block(block_number, inode_number, 0,12)
                        remaining_blocks -= count
                if remaining_blocks > 0:
                    #doubly indirect blocks
                    block_number = int(line[24],16)
                    if block_number == 0 or block_number > self._block_count:
                        self._invalid_blocks.append((block_number, inode_number, 0,13))
                    else:
                        count = self.scan_doubly_indirect_block(block_number,inode_number, 0,13)
                        remaining_blocks -= count
                
                
                if remaining_blocks > 0:
                    #triply indirect blocks
                    block_number = int(line[25],16)
                    if block_number == 0 or block_number > self._block_count:
                        self._invalid_blocks.append((block_number,inode_number,0,14))
                    else:
                        count = self.scan_triply_indirect_block(block_number,inode_number,0,14)
                        remaining_blocks -= count
        return
    #scan indirect blocks
    def scan_indirect_block(self, block_number, inode_number, indirect_block_number,entry_num):
        count = 1
        self.increment_block_referenced_by(block_number, inode_number,indirect_block_number,entry_num)
        if block_number in self._indirect_dict:
            for entry in self._indirect_dict[block_number]:
                self.increment_block_referenced_by(entry[1],inode_number,block_number,entry[0])
                count += 1
        return count
    def scan_doubly_indirect(self,block_number,inode_number,indirect_block_number,entry_num):
        count = 1
        self.increment_block_referenced_by(block_number, inode_number, indirect_block_number,entry_num)
        if block_number in self._indirect_dict:
            for entry in self._indirect_dict[block_number]:
                count += self.scan_indirect_block(entry[1],inode_number,block_number,entry[0])
        return count

    def scan_triply_indirect_block(self,block_number,inode_number,indirect_block_number,entry_num):
        count = 1
        self.increment_block_referenced_by(block_number,inode_number,indirect_block_number,entry_num)
        if block_number in self._indirect_dict:
            for entry in self._indirect_dict[block_number]:
                count += self.scan_doubly_indirect_block(entry[1],inode_number,block_number,entry[0])
        return count

    def increment_block_referenced_by(self,block_number,inode_number,indirect_block_number,entry_num):
        if block_number == 0 or block_number > self._block_count:
            self._invalid_blocks.append((block_number,inode_number,indirect_block_number,entry_num))
        else:
            if block_number not in self._block_dict:
                self._block_dict[block_number] = Block(block_number)
            self._block_dict[block_number]._referenced_by.append((inode_number,indirect_block_number,entry_num))
        return

    def read_files(self):
        self.read_super()
        self.read_group()
        self.read_bitmap()
        self.read_indirect()
        self.read_inode()
        self.read_directory()
        return

    def report_errors(self,file_name = "lab3b_check.txt"):
        output_file = open(file_name,'w')

        for entry in sorted(self._unallocated_inodes):
            temp_buff = "UNALLOCATED INODE < " + str(entry) + " > REFERENCED BY "
            for i in sorted(self.unallocated_inodes[entry]):
                temp_buff += "DIRECTORY < "+ str(i[0])+ " > ENTRY < " + str(i[1])+ " > "
                
                output_file.write(temp_buff.strip()+ "\n")
        
        for entry in sorted(self._incorrect_entries):
            output_file.write("INCORRECT ENTRY IN < " + str(entry[0]) + " > NAME < " + str(entry[1]) + " > LINK TO < " + str(entry[2]) + " > SHOULD BE < " + str(entry[3]) + " >\n")
        for entry in sorted(self._inode_dict):
            link_count = len(self._inode_dict[entry]._referenced_by)

            if entry > 10 and link_count == 0 and entry != 15 and entry != 22:
                bitmap_block_num = self._inode_bitmap_blocks[entry/self._ipg]
                output_file.write("MISSING INODE < " + str(entry) + " > SHOULD BE IN FREE LIST < " + str(bitmap_block_num) + " >\n")
            elif link_count != self._inode_dict[entry]._link_count:
                output_file.write("LINKCOUNT < " + str(entry) + " > IS < " + str(self._inode_dict[entry]._link_count)+ " > SHOULD BE < " + str(link_count) + " >\n")



            if entry in self._inode_free_list:
                output_file.write("UNALLOCATED INODE < " + str(entry) + " >\n")

        for j in range(11,self._inode_count):
            if j not in self._inode_free_list and j not in self._inode_dict:
                bitmap_block_num = self._inode_bitmap_blocks[j/self._ipg]
                output_file.write("MISSING INODE < " + str(j) + " > SHOULD BE IN FREE LIST < " + str(bitmap_block_num) + " >\n")

        for entry in sorted(self._block_dict):
            if len(self._block_dict[entry]._referenced_by) > 1:
                temp_buff = "MULTIPLY REFERENCED BLOCK < " + str(entry) + " > BY "
                for i in sorted(self._block_dict[entry]._referenced_by):
                    if i[1] == 0:
                        temp_buff += "INODE < " + str(i[0]) + " > ENTRY < " + str(i[2]) + " > "
                    else:
                        temp_buff += "INODE < " + str(i[0]) + " > INDIRECT BLOCK < " + str(i[1]) + " > ENTRY < " + str(i[2]) + " > "
                output_file.write(temp_buff.strip() + "\n")
            if entry in self._block_free_list:
                temp_buff = "UNALLOCATED BLOCK < " + str(entry) + " > REFERENCED BY "
                for i in sorted(self._block_dict[entry]._referenced_by):
                    if i[1] == 0:
                        temp_buff += "INODE < " + str(i[0]) + " > ENTRY < " + str(i[2]) + " > "
                    else:
                        temp_buff += "INODE < " + str(i[0]) + " > INDIRECT BLOCK < " + str(i[1]) + " > ENTRY < " + str(i[2]) + " > "
                output_file.write(temp_buff.strip() + "\n")
        #perhaps edit this later on 
        for entry in sorted(self._invalid_blocks):
            output_file.write("INVALID BLOCK < " + str(entry[0]) + " > IN INODE < " + str(entry[1]) + " >" + " ENTRY < " + str(entry[3]) + " >\n")

        output_file.close()
        return

if __name__ == "__main__":
    diagnostics = csv_parser()
    diagnostics.read_files()
    diagnostics.report_errors()
