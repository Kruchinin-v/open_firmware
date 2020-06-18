#include <iostream>
#include <fstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>

using namespace std;

uint_least32_t Crc32(char* buf, size_t len)
{
	uint_least32_t crc_table[256];
	uint_least32_t crc;
	int i, j;

	for (i = 0; i < 256; i++)
	{
		crc = i;
		for (j = 0; j < 8; j++)
			crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;

		crc_table[i] = crc;
	};

	crc = 0xFFFFFFFFUL;

	while (len--)
		crc = crc_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);

	return crc ^ 0xFFFFFFFFUL;
}

int read32(int point, char* filename)//функция считывания 32 битов из файла
{
	fstream file;
	file.open(filename, ios::binary | ios::in); //открытие файла источника

	char tmp[1];
	int rez;
	for (int i = 0; i < 4; i++)
	{
		file.seekg(point--, ios::beg);
		file.read(tmp, 1);
		rez = (rez << 8) | (int)(tmp[0] & 0xff);
	}
	file.close();
	return rez;
}

int write32(int point, const char* filename, int x)//функция записи 32 битов в файл
{
	fstream file;
	file.open(filename, ios::binary | ios::in | ios::out); //открытие файла источника

	char tmp[1];
	int rez;

	file.seekp(point--, ios::beg);//первый байт
	tmp[0] = rez = (x >> 8 * 3) & 0xff;
	file.write(tmp, 1);

	file.seekp(point--, ios::beg);//второй байт
	tmp[0] = (x >> 8 * 2) & 0xffffff;
	file.write(tmp, 1);

	file.seekp(point--, ios::beg);//третий байт
	tmp[0] = (x >> 8) & 0xffff;
	file.write(tmp, 1);

	file.seekp(point--, ios::beg);//третий байт
	tmp[0] = x & 0xff;
	file.write(tmp, 1);

	rez = (rez << 8) | (int)(tmp[0] & 0xff);
	file.close();
	return 0;
}

int clear32(int point, const char* filename)//функция очистки 32 битов в файле
{
	fstream file;
	file.open(filename, ios::binary | ios::in | ios::out); //открытие файла источника

	char tmp[1];
	tmp[0] = 0x00;
	for (int i = 0; i < 4; i++)
	{
		file.seekp(point--, ios::beg);
		file.write(tmp, 1);
	}
	return 0;
}

int main(int argc, char** argv)
{
	char* input;//переменная для хранения имени файла
	input = argv[2];
	const char* output1 = "header_file", * output = "newfirmware.bin", * output3 = "squashfs_file", * insqash = "newsquashfs";
	//insqash - собранный sqashfs файл


	const int SIZE = 5000;
	char buf[SIZE];
	int outlength;//длина файла выходного файла
	int length;//оставшаяся длина считывания
	int p; //адрес ячейки с началом 3 части
	int siz;//адрес оффсета, начало 3 части
	int sz;// размер считывания 

	std::fstream infile; //файл источник
	std::fstream outfile; //программа

	std::map <std::string, int> arg;
	arg["def"] = 0;//очистка временных и распакованных файлов
	arg["--pack"] = 1;//упаковка 
	arg["--unpack"] = 2;//распаковка
	arg["--clear"] = 3;//очистка временных и распакованных файлов
	arg["--clearall"] = 3;//очистка временных, распакованных файлов и новой прошивки


	std::map <std::string, int> ::iterator it, it_2;

	if (argc >= 2)//if указаны аргументы
		it = arg.find(argv[1]);
	else
		it = arg.find("def");

	switch (it->second)
	{
	case 1://pack
	{
		int stat;
		//проверка налиячия папки root
		stat = system("ls squashfs-root > /dev/null 2>&1");

		//проверка открытия файла шапки с ядром
		if (stat == 512)
		{
			std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
			std::cerr << "Folder squashfs-root not found." << endl;
			std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
			return 0;
		}//if

		//проверка налиячия файла шапки
		stat = system("ls header_file > /dev/null 2>&1");		
		
		//проверка открытия файла шапки с ядром
		if (stat==512)
		{
			std::cerr << "~~~~~~~~~~~~~~~~~~~~~~" << endl;
			std::cerr << "Header file not found." << endl;
			std::cerr << "~~~~~~~~~~~~~~~~~~~~~~" << endl;
			return 0;
		}//if

		system("rm -f newsquashfs");
						
		stat = system("mksquashfs squashfs-root/ newsquashfs -comp xz 2>./log_mksquashfs");//упаковка

		//проверка выполнения mksquashfs
		if (stat)
		{
			std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
			std::cerr << "Packaging error squashfs" << std::endl;
			std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
			return 0;
		}
		else system("rm -f log_mksquashfs");


		outfile.open(output, std::ios::binary | std::ios::out); //открытие файла записи

		//проверка создания нового bin файла 
		if (!outfile)
		{
			std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
			std::cerr << "Error creating new bin file!" << endl;
			std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
			return 0;
		}//if

		//перенос первой части
		{//--------------------------------------------------------------------------перенос первой части
						
			infile.open(output1, ios::binary | ios::in); //открытие файла источника

			if (!outfile)
			{
				std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
				std::cerr << "File '" << output1 << "' cant't be oppened." << endl;
				std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
				return 0;
			}//if

			infile.seekg(0, ios::end);
			outlength = infile.tellg();//размер читаемого файла

			length = outlength;

			infile.seekg(0, ios::beg);
			sz = 1024;//по сколько байт считывать
			while (length >= sz)//считывать, пока хватает длины необходимого размера
			{
				infile.read(buf, sz);
				outfile.write(buf, sz);
				length -= sz;
			}

			if (length > 0)//если осталась длина, считать и ее
			{
				infile.read(buf, length);
				outfile.write(buf, length);
			}
			infile.close();
			//--------------------------------------------------------------------------перенос первой части
		}

		//перенос второй части
		{
			infile.open(insqash, ios::binary | ios::in); //открытие файла источника

			infile.seekg(0, ios::end);
			outlength = infile.tellg();//размер читаемого файла

			length = outlength;

			infile.seekg(0, ios::beg);
			sz = 1024;//по сколько байт считывать
			while (length >= sz)//считывать, пока хватает длины необходимого размера
			{
				infile.read(buf, sz);
				outfile.write(buf, sz);
				length -= sz;
			}

			if (length > 0)//если осталась длина, считать и ее
			{
				infile.read(buf, length);
				outfile.write(buf, length);
			}
			infile.close();
		}

		outfile.close();

		//записать размер 3 части
		{
			p = 295; //адрес оффсета с размером 3 части
			clear32(p, output);//очистить старый размер

			//запись размера 3 части
			write32(p, output, outlength);
		}

		//замена crc32 3 части
		{
			//поиск начала 3 части - siz
			{
				p = 291;//адрес оффсета с началом 3 части
				char cc[20];
				strncpy(cc, output, 20);//преобразование имени файла
				siz = read32(p, cc);//вызов функции считывания 32 бит адреса 3 части
			}

			if (outlength > 20000000)
			{
				std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
				std::cerr << "Root size exceeds acceptable!" << endl;
				std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
				return 0;
			}

			char* bufcrc = new char[20000000];

			infile.open(output, ios::binary | ios::in);
			infile.seekg(siz, ios::beg);
			infile.read(bufcrc, outlength);

			int crc32;

			crc32 = Crc32(bufcrc, outlength);
			delete[] bufcrc;

			//cout << hex << "crc32 3 part: " << crc32 << endl;
			p = 555;//адрес оффсета crc32 3 части
			write32(p, output, crc32);
			infile.close();
		}

		//замена итогового размера файла
		{
			p = 15;
			clear32(p, output);

			infile.open(output, ios::binary | ios::in | ios::out);

			infile.seekg(0, ios::end);
			outlength = infile.tellg();//размер всего файла

			write32(p, output, outlength);

			infile.close();
		}

		//подсчет итоговый crc32 шапки
		{
			p = 11;
			clear32(p, output);

			infile.open(output, ios::binary | ios::in | ios::out);

			outlength = 1556;//размер считывания
			infile.read(buf, outlength);
			int crc32 = Crc32(buf, outlength);
			write32(p, output, crc32);

		}
		std::cout << "~~~~~~~~~~~~~~~~~~~~~~~" << endl;
		std::cout << "Pack successfully" << endl;
		std::cout << "Result: newfirmware.bin" << endl;
		std::cout << "~~~~~~~~~~~~~~~~~~~~~~~" << endl;
		system("rm -rf newsquashfs && ls -l --color=auto");
	}
	break;
	case 2://unpack
	{
		//!!!unpack------------------------------------------------------------------------------------------------------!!!unpack

		if (!argv[2])
		{
			std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
			std::cerr << "Error: You must specify a file name!" << endl;
			std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
			return 0;
		}

		//копирование 12 части
		{
			//-----------------------------------------------------------------------------------------------копирование 12 части

			infile.open(input, ios::binary | ios::in); //открытие файла источника

			//проверка открытия указанного файла
			if (!infile)
			{
				std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
				std::cerr << "File '" << input << "' cant't be oppened." << endl;
				std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
				return 0;
			}//if

			outfile.open(output1, ios::binary | ios::out); //открытие файла для записи программы

			//проверка создания файла шапки
			if (!outfile)
			{
				std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
				std::cerr << "Error creating header file!" << endl;
				std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
				return 0;
			}//if

			p = 291;//адрес оффсета с началом 3 части
			siz = read32(p, input);//вызов функции считывания 32 бит адреса 3 части

			p = 295;//адрес оффсета с размером 3 части
			outlength = read32(p, input);//вызов функции считывание 32 бит размера 3 части

			length = siz;//получения количества байт для считывания

			sz = 1024;//по сколько байт считывать
			while (length >= sz)//считывать, пока хватает длины необходимого размера
			{
				infile.read(buf, sz);
				outfile.write(buf, sz);
				length -= sz;
			}

			if (length > 0)//если осталась длина, считать и ее
			{
				infile.read(buf, length);
				outfile.write(buf, length);
			}

			outfile.close();
			//-----------------------------------------------------------------------------------------------копирование 12 части
		}

		//копирование 3 части
		{
			//-----------------------------------------------------------------------------------------------копирование 3 части

			outfile.open(output3, ios::binary | ios::out); //открытие файла для записи программы

			//проверка создания файла squashfs
			if (!outfile)
			{
				std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
				std::cerr << "Error creating sqashfs file!" << endl;
				std::cerr << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
				return 0;
			}//if

			length = outlength;//получения количества байт для считывания

			infile.seekg(siz, ios::beg);


			while (length >= sz)//считывать, пока хватает длины необходимого размера
			{
				infile.read(buf, sz);
				outfile.write(buf, sz);
				length -= sz;
			}

			if (length > 0)//если осталась длина, считать и ее
			{
				infile.read(buf, length);
				outfile.write(buf, length);
			}
			//-----------------------------------------------------------------------------------------------копирование 3 части
		}

		//удаление существующей папки
		system("rm -rf squashfs-root");

		int stat;
		stat = system("unsquashfs squashfs_file");//распаковка полученного squashfs файла

		//if успешно распакованно
		if (!stat)
		{
			std::cout << endl << "~~~~~~~~~~~~~~~~~~~" << endl;
			std::cout << "Unpack successfully" << endl;
			std::cout << "~~~~~~~~~~~~~~~~~~~" << endl;
			system("rm -rf squashfs_file");
			system("ls -l --color=auto");
		}
		//else ошибка упаковки
		else
		{
			std::cerr << "~~~~~~~~~~~~~~~~" << endl;
			std::cerr << "Error unsquashfs" << endl;
			std::cerr << "~~~~~~~~~~~~~~~~" << endl;
		}


		
		infile.close();
		outfile.close();
		//!!!unpack------------------------------------------------------------------------------------------------------!!!unpack
	}
	break;
	case 3://clear
	{
		int stat;//strcmp

		

		if (!strcmp(argv[1],"--clear"))
		{
			stat = system("rm -rf ./squashfs_file  ./squashfs-root ./header_file ./newsquashfs log_mksquashfs");//удаление временных файлов и папок
		}
		else
			stat = system("rm -rf ./squashfs_file  ./squashfs-root ./header_file ./newsquashfs ./newfirmware.bin log_mksquashfs");//удаление временных файлов и папок

		if (!stat)
		{
			std::cout << "~~~~~~~~~~~~~~~~~~~" << endl;
			std::cout << "Clear successfully" << endl;
			std::cout << "~~~~~~~~~~~~~~~~~~~" << endl;
		}
		//else ошибка удаления
		else
		{
			std::cerr << "~~~~~~~~~~~" << endl;
			std::cerr << "Error clear" << endl;
			std::cerr << "~~~~~~~~~~~" << endl;
		}

	}
	break;
	default://def
	{
		std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
		std::cout << "Help:" << endl;
		std::cout << "--unpack path   for unpacking;" << endl;
		std::cout << "--pack          for package;" << endl;
		std::cout << "--clear         for clear files." << endl;
		std::cout << "--clearall      for clear files and newfirmware." << endl;
		std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
		return 0;
	}
	break;
	}
}