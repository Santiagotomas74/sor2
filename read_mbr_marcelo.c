#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *in = fopen("test.img", "rb"); // Abrimos el archivo en modo binario
    unsigned int i, start_sector, length_sectors;

    if (in == NULL) {
        perror("Error abriendo el archivo");
        return 1;
    }

    // Vamos al byte 446, donde comienza la tabla de particiones (MBR es de 512 bytes)
    fseek(in, 446, SEEK_SET); // Desplazamos el puntero a la posición 446

    for(i = 0; i < 4; i++) { // Leemos las 4 entradas de la tabla de particiones, eliminar esta linea para el punto 2.d
        printf("Partition entry %d:\n", i + 1); // cambiar a printf("Partition entry :\n"); para el punto 2.d
        
        // Leer el primer byte de la entrada de partición (flag booteable)
        unsigned char bootable_flag = fgetc(in);
        printf("  First byte: 0x%02X (%s)\n", bootable_flag,  bootable_flag == 0x80 ? "booteable" : "no booteable");
        
        // Leer la dirección CHS de inicio (3 bytes)
        unsigned char chs_start[3];
        chs_start[0] = fgetc(in);
        chs_start[1] = fgetc(in);
        chs_start[2] = fgetc(in);
        unsigned char head = chs_start[0];
        unsigned char sector = chs_start[1] & 0x3F; // 6 bits bajos
        unsigned int cylinder = ((chs_start[1] & 0xC0) << 2) | chs_start[2];
        printf("  CHS inicio: Cabeza: %d, Sector: %d, Cilindro: %d\n", head, sector, cylinder);
        
        // Leer el tipo de partición (1 byte)
        unsigned char partition_type = fgetc(in);
        printf("  Tipo de partición: 0x%02X\n", partition_type);
        
        // Leer la dirección CHS de fin (3 bytes)
        unsigned char chs_end[3];
        chs_end[0] = fgetc(in);
        chs_end[1] = fgetc(in);
        chs_end[2] = fgetc(in);
        unsigned char head_end = chs_end[0];
        unsigned char sector_end = chs_end[1] & 0x3F; // bits bajos (1-63)
        unsigned int cylinder_end = ((chs_end[1] & 0xC0) << 2) | chs_end[2];
        printf("  CHS fin: Cabeza: %d, Sector: %d, Cilindro: %d\n", head_end, sector_end, cylinder_end);
        
        // Leer la dirección LBA de inicio (4 bytes) y el tamaño en sectores (4 bytes)
        fread(&start_sector, 4, 1, in); // Dirección LBA de inicio
        fread(&length_sectors, 4, 1, in); // Tamaño en sectores
        printf("  Dirección LBA de inicio: 0x%08X\n", start_sector);
        printf("  Tamaño en sectores: %d\n\n", length_sectors);
    } // borrar linea para el 2.d
    
    fclose(in); // Cerramos el archivo
    return 0;
}