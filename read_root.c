#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned char first_byte;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char end_chs[3];
    char starting_cluster[4];
    char file_size[4];
} __attribute((packed)) PartitionTable;

typedef struct {
    unsigned char jmp[3];
    char oem[8];
    unsigned short sector_size;
    //completado
	unsigned char sectors_per_cluster;
    unsigned short reserved_sectors;
    unsigned char number_of_fats;
    unsigned short root_dir_entries;
    unsigned short total_sectors;
    unsigned char media_descriptor;
    unsigned short fat_size_sectors;
    unsigned short sectors_per_track;
    unsigned short number_of_heads;
    unsigned int hidden_sectors;
    unsigned int total_sectors_large;
    unsigned char drive_number;
    unsigned char reserved1;
    unsigned char boot_signature;
    //terminando completado
    unsigned int volume_id;
    char volume_label[11];
    char fs_type[8];
    char boot_code[448];
    unsigned short boot_sector_signature;
} __attribute((packed)) Fat12BootSector;

typedef struct {
    unsigned char filename[8];      // Nombre del archivo
    char extension[3];           // Extensión del archivo
    unsigned char attr;    // Atributos del archivo
    unsigned char reserved;
    unsigned char creation_time_ms;
    unsigned short creation_time;
    unsigned short creation_date;
    unsigned short last_access_date;
    unsigned short high_cluster; 
    unsigned short last_mod_time;
    unsigned short last_mod_date;
    unsigned short low_cluster; 
    unsigned int file_size;
} __attribute((packed)) Fat12Entry;

void print_file_info(Fat12Entry *entry) {
    if((entry->attr & 0x0F) == 0x0F){
        printf("LFN entry: [%.13s]\n", entry->filename)
        return;
    } 
    switch(entry->filename[0]) {
        case 0x00:
            return; // Entrada no usada (final de la tabla)
        case 0xE5:
            printf("Archivo borrado: [?%.7s.%.3s]\n", entry->filename, entry->extension);
            return;
        case 0x2E:
            printf("Entrada especial: [.%.7s.%.3s]\n", entry->filename, entry->extension);
            return;
        default:
            if (entry->attr & 0x10) {
                printf("Directorio: [%.8s.%.3s]\n", entry->filename, entry->extension);
            } else {
                printf("Archivo: [%.8s.%.3s] - Tamaño: %u bytes\n", entry->filename, entry->extension, entry->file_size);
            }
        }
    
}
void recover_file(Fat12Entry *entry, long offset, FILE *in, int recover) {
    if((entry->attr & 0x0F) == 0x0F) return;
    switch(entry->filename[0]) {
        case 0xE5:
            fseek(in, offset, SEEK_SET);
            unsigned char first_char = 'R'; // Cambia '?' por 'R' (puede ser cualquier letra válida)
            fwrite(&first_char, 1, 1, in);
            fflush(in);
            printf("Archivo borrado recuperado como: [?%.7s.%.3s] Easy Peasy\n", entry->filename, entry->extension);
                
            fseek(in, offset + sizeof(Fat12Entry), SEEK_SET); // Nos movemos a la siguiente entrada

            return;
        default:
            return;
        }
    
}
int main(int argc, char *argv[]) {
    int recover = (argc > 1 && strcmp(argv[1], "recover") == 0);
    FILE * in = fopen("test.img", "rb");
    if (!in) {
        perror("Error al abrir el archivo");
        return -1;
    }
    
    int i;
    PartitionTable pt[4];
    Fat12BootSector bs;
    Fat12Entry entry;
   
	    // Leer tabla de particiones
        if (fread(pt, sizeof(PartitionTable), 4, in) != 4) {
            perror("Error leyendo tabla de particiones");
            fclose(in);
            return -1;
        }

    
    // Buscar partición FAT12
    for(i=0; i<4; i++) {     
        printf("partition_type 0x%X\n", (unsigned int)pt.partition_type);   
        if(pt[i].partition_type == 8) {
            printf("Encontrada particion FAT12 %d\n", i);
            break;
        }
    }
    
    if(i == 4) {
        printf("No encontrado filesystem FAT12, saliendo...\n");
        fclose(in);
        return -1;
    }
    
    // Mover el puntero al inicio del Boot Sector
    fseek(in, 0, SEEK_SET);

    if(fread(&bs, sizeof(Fat12BootSector),1,in) !=1){
        perror("Error leyendo Boot Sector");
        fclose(in);
        return -1;
    }

    // fread(pt, sizeof(PartitionTable), 4, in);
    // fseek(in, 0, SEEK_SET);

	//{...} Leo boot sector
    
    printf("En  0x%X, sector size %d, FAT size %d sectors, %d FATs\n\n", 
           (unsigned int)ftell(in), bs.sector_size, bs.fat_size_sectors, bs.number_of_fats);
    
    // Posicionar el puntero al directorio raíz
    // Fórmula: sectores reservados - 1 + (Tamaño total de FATs)
    fseek(in, (bs.reserved_sectors-1 + bs.fat_size_sectors * bs.number_of_fats) *
          bs.sector_size, SEEK_CUR);
    
    printf("Root dir_entries %d \n", bs.root_dir_entries);
    for(i=0; i<bs.root_dir_entries; i++) {
        long entry_offset = ftell(in);
        if (fread(&entry, sizeof(Fat12Entry), 1, in) != 1) break;
        print_file_info(&entry, entry_offset, in, recover);
        if(recover) recover_file(&entry, entry_offset, in, recover);
    }
    
    printf("\nLeido Root directory, ahora en 0x%X\n", (unsigned int)ftell(in));
    fclose(in);
    return 0;
}
