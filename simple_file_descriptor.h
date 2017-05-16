#pragma once

struct simple_file_descriptor
{
	struct pointer
	{
		pointer(const pointer& another);
		pointer(pointer&& another);
		pointer(int fd);

		int operator*() const;

		pointer& operator=(const pointer&);
		
		private:

		int fd;
		
		friend simple_file_descriptor;
	};
	
	simple_file_descriptor(int fd);
	simple_file_descriptor(simple_file_descriptor&& another);
	virtual ~simple_file_descriptor();
	
	int operator*() const;

	pointer get_pointer() const;

	protected:
	bool is_valid();
	
	bool valid;
	int fd;
};

bool operator<(const simple_file_descriptor&, const simple_file_descriptor&);
bool operator<(const simple_file_descriptor::pointer&, const simple_file_descriptor::pointer&);
