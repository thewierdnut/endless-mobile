/* File.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "File.h"

#include "Files.h"



using namespace std;



File::File(const string &path, bool write)
{
	file = Files::Open(path, write);
}



File::File(File &&other) noexcept
{
	swap(file, other.file);
}



File::~File() noexcept
{
	if(file)
		Files::Close(file);
}



File &File::operator=(File &&other) noexcept
{
	if(this != &other)
		swap(file, other.file);
	return *this;
}



File::operator bool() const
{
	return file;
}



File::operator struct SDL_RWops*() const
{
	return file;
}
