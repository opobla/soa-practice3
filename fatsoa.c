/*
    Fichero: fatfs.c
    Copyright 2023. The Operating System Team
 */

#include "./fatsoa.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "./parser.h"

#define PROMPT_STRING "FATFS:"

int fd;
char current_path[512] = "/";
uint32_t current_dir_cluster = 2;

ssize_t bytes_read;
uint32_t fat_begin_offset = 0;  // Desplazamiento de la FAT en la imagen

struct BS_Structure bs_data;  // Informaci'on del sector de arranque (boot
                              // sector)
struct DIR_Structure directory_info[16];  // Un sector, los 16 primeras entradas
                                          // de un directorio

char prompt[512];

/* --------------------------------------------------------------
    fat_entry ()

    Imprime y devuelve el contenido de una entrada de la FAT dado
    el numero de cluster.

    Parámetro:
        cluster     Numero de cluster

    Resultado:
        Entrada de la FAT
   -------------------------------------------------------------- */
uint32_t get_fat_entry(uint32_t cluster) {
  uint32_t fat_entry;

  lseek(fd, fat_begin_offset + (cluster << 2), SEEK_SET);
  bytes_read = read(fd, &fat_entry, sizeof(fat_entry));

  printf("FAT entry %u: 0x%08X\n", cluster, fat_entry);

  return fat_entry;
}

/* --------------------------------------------------------------
    LBA2Offset ()

    Calcula el desplazamiento (offset ) del cluster que recibe como
    parametro.

    Parámetro:
        cluster     Numero de cluster.

    Resultado:
        Offset del cluster dentro del volumen FAT32

    Antes de usar esta función es preciso leer el sector de boot
    y rellenar la estructura bs_data.
   -------------------------------------------------------------- */
uint32_t LBA2Offset(uint32_t cluster) {
  uint32_t offset;

  if (cluster <= 1) {
    // printf("Invalid sector value %d\n", cluster );
    cluster = 2;
  }

  offset =
      ((cluster - 2) * bs_data.bytesPerSector * bs_data.sectorPerCluster) +
      (bs_data.bytesPerSector * bs_data.reservedSectorCount) +
      (bs_data.numberofFATs * bs_data.FATsize_F32 * bs_data.bytesPerSector);

  return offset;
}

/* --------------------------------------------------------------
    fs_get ()

    Extrae el fichero cuyo nombre se pasa como parametro.

    Parámetro:
        file_get     Puntero al nombre del fichero a extraer.

    El usuario proporciona el nombre como <nombre>.<extension> sin
    embargo en la informacion del directorio los nombres se guardan
    en un array de 11 caracteres (8 para el nombre y 3 para la
    extension) sin el punto. Si el nombre no ocupa los 8 caracteres
    se rellena el final con caracteres en blanco. Para compararlos
    se modifica el nombre del fichero proporcionado por el usuaro
    al formato almacenado en el directorio.

    Por ejemplo:
        "LEEME.TXT"     =>  "LEEME   TXT"
        "SISTEMAS.DOC"  =>  "SISTEMASDOC"
   -------------------------------------------------------------- */
void fs_get(char *file_get) {
  // 11 potential characters pull the null-terminator
  char file_name[11 + 1];

  // 11 potential characters, plus the dot and the null-terminator
  char file_get_modificado[11 + 1 + 1];
  int i = 0, file_found = 0;
  char *p_punto = NULL;

  if (file_get == NULL) {
    return;
  }

  p_punto = strchr(file_get, '.');

  if (p_punto) {
    // It it starts with a dot its a relative path
    *p_punto = '\0';
    memset(file_get_modificado, ' ', 11);
    memcpy(file_get_modificado, file_get, strlen(file_get));
    memcpy(&file_get_modificado[8], p_punto + 1, strlen(p_punto + 1));

    *p_punto = '.';
  } else {
    strcpy(file_get_modificado, file_get);
  }

  printf("File name modificado: %s-[%zu]\n", file_get_modificado,
         strlen(file_get_modificado));

  // sino hay punto lo comparamos tal cual
  do {
    if (directory_info[i].DIR_name[0] != (uint8_t)0xe5) {
      if (directory_info[i].DIR_attrib & ATTR_ARCHIVE) {
        memcpy(file_name, (const char *)directory_info[i].DIR_name, 11);
        file_name[11] = '\0';

        printf("File name: %s-[%zu]\n", file_name, strlen(file_name));

        if (strcmp(file_name, file_get_modificado) == 0) {
          file_found = 1;
        }
      }
    }
    i++;
  } while (!file_found && (i < 16));

  if (!file_found) {
    printf("%s not found\n", file_get);
  } else {
    printf("%s found\n", file_get);

    int32_t file_cluster = (directory_info[i - 1].firstClusterHI << 16) +
                           (directory_info[i - 1].firstClusterLO);

    int32_t file_offset = LBA2Offset(file_cluster);

    printf("File: cluster inicio: 0x%X offset: 0x%X\n", file_cluster,
           file_offset);

    // TODO(by the student):
    //      Copiar al exterior y con el mismo nombre el fichero que se encuentra
    //      a partir del offset teniendo en cuenta el tama;o y que puede ocupar
    //      uno o mas clusters
  }
}

/* --------------------------------------------------------------
    print_attrib ()

    Imprime el significado del byte de atributos de una entrada
    de directorio.

    Parámetro:
        attrib     Byte de atributos
   -------------------------------------------------------------- */
void print_attrib(uint8_t attrib) {
  if (attrib == ATTR_LONG_NAME) {
    printf("       - Long name\n");
  } else {
    if (attrib & ATTR_READ_ONLY) printf("       - Read only\n");

    if (attrib & ATTR_HIDDEN) printf("       - Hidden\n");

    if (attrib & ATTR_READ_ONLY) printf("       - Read only\n");

    if (attrib & ATTR_SYSTEM) printf("       - System\n");

    if (attrib & ATTR_VOLUME_ID) printf("       - Volume ID\n");

    if (attrib & ATTR_DIRECTORY) printf("       - Directory\n");

    if (attrib & ATTR_ARCHIVE) printf("       - Archive\n");
  }
}

/* --------------------------------------------------------------
    fs_stat ()

    Imprime el contenido detallado de las entradas del directorio
    actual.

    Parámetro:
        cluster     Numero de cluster

   Antes de usar esta función es preciso leer el contenido de un
   directorio y rellenar la estructura directory_info. En la funcion
   "open" se lee el direcorio raiz y en la funci'on "cd" se lee
   cada vez que se cambia de directorio.
   -------------------------------------------------------------- */
void fs_stat() {
  char dir_name[12];
  int i;

  for (i = 0; i < 16; i++) {
    if (directory_info[i].DIR_name[0] != (uint8_t)0xe5) {
      if (directory_info[i].DIR_attrib != 0) {
        memcpy(dir_name, (const char *)directory_info[i].DIR_name, 11);
        dir_name[11] = '\0';
        printf("-------------------------\n");
        printf("DIR name: %s\n", dir_name);
        printf("DIR attrib: %u\n", (uint32_t)directory_info[i].DIR_attrib);
        print_attrib(directory_info[i].DIR_attrib);
        printf("DIR firstClusterHI: %u\n",
               (uint32_t)directory_info[i].firstClusterHI);
        printf("DIR firstClusterLO: %u\n",
               (uint32_t)directory_info[i].firstClusterLO);
        printf("Image offset: 0x%X\n",
               LBA2Offset((directory_info[i].firstClusterHI << 16) +
                          directory_info[i].firstClusterLO));
        printf("DIR fileSize: %u [%X]\n", (uint32_t)directory_info[i].fileSize,
               (uint32_t)directory_info[i].fileSize);
      }
    }
  }
}

/* --------------------------------------------------------------
    fs_ls ()

    Imprime el listado del contenido del directorio actual.
   -------------------------------------------------------------- */
void fs_ls() {
  char dir_name[12];
  int i;

  for (i = 0; i < 16; i++) {
    if (directory_info[i].DIR_name[0] != (uint8_t)0xe5) {
      if (directory_info[i].DIR_attrib & ATTR_DIRECTORY) {
        memcpy(dir_name, (const char *)directory_info[i].DIR_name, 11);
        dir_name[11] = '\0';
        printf("<DIR> %s\n", dir_name);
      } else if (directory_info[i].DIR_attrib & ATTR_ARCHIVE) {
        memcpy(dir_name, (const char *)directory_info[i].DIR_name, 11);
        dir_name[11] = '\0';
        printf("%s\n", dir_name);
      }
    }
  }
}

/* --------------------------------------------------------------
    fs_cd ()

    Cambia el directorio actual y rellena la estructura
    directory_info.
   -------------------------------------------------------------- */
void fs_cd(char *new_dir) {
  char dir_name[12], *p;
  int i = 0, dir_found = 0;
  uint32_t offset;

  do {
    if (directory_info[i].DIR_name[0] != (uint8_t)0xe5) {
      if (directory_info[i].DIR_attrib & ATTR_DIRECTORY) {
        memcpy(dir_name, (const char *)directory_info[i].DIR_name, 11);
        dir_name[11] = '\0';

        p = strchr(dir_name, ' ');
        *p = '\0';

        if (strcmp(dir_name, new_dir) == 0) {
          current_dir_cluster = (directory_info[i].firstClusterHI << 16) +
                                directory_info[i].firstClusterLO;

          offset = LBA2Offset(current_dir_cluster);

          lseek(fd, offset, SEEK_SET);
          bytes_read = read(fd, directory_info, sizeof(directory_info));

          if (bytes_read != sizeof(directory_info)) {
            printf("directory_info on drive offset %u not read\n", offset);
          }

          dir_found = 1;
        }
      }
    }

    i++;
  } while (!dir_found && (i < 16));

  if (!dir_found) {
    printf("%s not found\n", new_dir);
  } else {
    // update current path and prompt
    if (strcmp(new_dir, "..") == 0) {
      p = strrchr(current_path, '/');

      if (p != current_path) {
        *p = '\0';
      } else {
        *(p + 1) = '\0';
      }
    } else {
      if (strcmp(current_path, "/") != 0) {
        strcat(current_path, "/");
      }

      strcat(current_path, new_dir);
    }

    strcpy(prompt, PROMPT_STRING);
    strcat(prompt, current_path);
    strcat(prompt, " ");
  }
}

/* --------------------------------------------------------------
    fs_volumen ()

    Imprime la informacion del Boot Sector. Esta informacion se
    lee en la funcion open al abrir el volumen.
   -------------------------------------------------------------- */
void fs_volumen(void) {
  char file_system_type[9];

  printf("\n-------------------------\n");
  strncpy(file_system_type, (const char *)bs_data.fileSystemType, 8);
  printf("File system type: %s\n", file_system_type);

  printf("Bytes per Sector: %u\n", (uint32_t)bs_data.bytesPerSector);
  printf("Sectors per Cluster: %u\n", (uint32_t)bs_data.sectorPerCluster);
  printf("Reserved Sectors Count: %u\n", (uint32_t)bs_data.reservedSectorCount);
  printf("Number of FATs: %u\n", (uint32_t)bs_data.numberofFATs);
  printf("FAT sectors size: %u\n", (uint32_t)bs_data.FATsize_F32);
  printf("FAT begin offset:      0x%04X\n", fat_begin_offset);
  printf("CLUSTERs begin offset: 0x%04X\n", LBA2Offset(2));
  printf("End Signature: 0x%04X\n", (uint16_t)bs_data.bootEndSignature);
  printf("-------------------------\n");
}

/* --------------------------------------------------------------
    open_file ()

    Realiza la apertura del fichero que contiene la imagen del
    volumen FAT32..
   -------------------------------------------------------------- */
int open_file(char *file_name) {
  uint32_t offset;

  fd = open(file_name, O_RDONLY);

  if (fd == -1) {
    printf("Image file %s does not exist\n", file_name);
    return 0;
  }

  printf("%s opened.\n", file_name);
  strcat(prompt, current_path);
  strcat(prompt, " ");

  bytes_read = read(fd, &bs_data, sizeof(struct BS_Structure));
  fat_begin_offset = bs_data.reservedSectorCount * bs_data.bytesPerSector;

  if (bytes_read != sizeof(struct BS_Structure)) {
    printf("Boot sector not read\n");
    close(fd);
    return 0;
  }

  // Despues del open y de leer los datos del sector de boot se lee la
  // informacion del directorio raiz
  offset = LBA2Offset(2);

  lseek(fd, offset, SEEK_SET);
  bytes_read = read(fd, directory_info, sizeof(directory_info));

  if (bytes_read != sizeof(directory_info)) {
    printf("directory_info on drive offset %u not read\n", offset);
    close(fd);
    return 0;
  }

  return 1;
}

int main(int argc, char *argv[]) {
  orden O;
  int res, opened;

  strcpy(prompt, PROMPT_STRING);

  printf("Introduzca órdenes (pulse Ctrl-D para terminar)\n");

  do {
    // Read and show the commands
    inicializar_orden(&O);

    printf("%s", prompt);
    res = leer_orden(&O, stdin);

    if (O.argc == 0) {
      continue;
    }

    if (res < 0) {
      fprintf(stderr, "\nError %d: %s\n", -res, mensajes_err[-res]);
    } else {
      // Process the command
      // mostrar_orden (&O);

      if (strcmp(O.argv[0], "open") == 0) {
        opened = open_file(O.argv[1]);
      } else if (strcmp(O.argv[0], "volumen") == 0) {
        if (opened) {
          fs_volumen();
        }
      } else if (strcmp(O.argv[0], "stat") == 0) {
        if (opened) {
          fs_stat();
        }
      } else if (strcmp(O.argv[0], "ls") == 0) {
        if (opened) {
          fs_ls();
        }
      } else if (strcmp(O.argv[0], "cd") == 0) {
        if (opened) {
          fs_cd(O.argv[1]);
        }
      } else if (strcmp(O.argv[0], "get") == 0) {
        if (opened) {
          fs_get(O.argv[1]);
        }
      } else if (strcmp(O.argv[0], "exit") == 0) {
        res = 1;
      } else {
        printf("Orden no conocida [%s]\n", O.orden_cruda);
      }
    }

    liberar_orden(&O);
  } while (res == 0);  // Repetir hasta error o EOF

  return 0;
}
