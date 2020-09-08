#include <iostream>
#include <string>
#include <vector>
#include <utility>

namespace nt
{
	enum
	{
		STATUS_SUCCESS = 0, STATUS_OBJECT_NAME_NOT_FOUND = 0xC0000034
	};
	static const ULONG OBJ_CASE_INSENSITIVE = 0x40;

	struct UNICODE_STRING
	{
		USHORT   Length, MaximumLength;
		wchar_t* Buffer;
	};

	struct OBJECT_ATTRIBUTES
	{
		ULONG           Length;
		HANDLE          RootDirectory;
		UNICODE_STRING* ObjectName;
		ULONG           Attributes;
		void            *SecurityDescriptor, *SecurityQualityOfService;

		OBJECT_ATTRIBUTES()
		{
			Length = sizeof(OBJECT_ATTRIBUTES);
			RootDirectory = 0;
			ObjectName = 0;
			Attributes = OBJ_CASE_INSENSITIVE;
			SecurityQualityOfService = SecurityDescriptor = 0;
		}
	};

	enum KEY_VALUE_INFORMATION_CLASS
	{
		KeyValueBasicInformation = 0, KeyValueFullInformation
	};
	enum KEY_INFORMATION_CLASS
	{
		KeyBasicInformation = 0
	};

	template <int max_length>
	struct KEY_VALUE_BASIC_INFORMATION
	{
		ULONG TitleIndex, Type, NameLength;
		wchar_t Name[max_length + 1];
	};
	template <int max_name_and_data_length>
	struct KEY_VALUE_FULL_INFORMATION
	{
		ULONG TitleIndex, Type, DataOffset, DataLength, NameLength;
		wchar_t Name[max_name_and_data_length + 2];
		const void* GetData() const
		{
			return (const void*)((const char*)this + DataOffset);
		}
	};
	template <int max_length>
	struct KEY_BASIC_INFORMATION
	{
		long long  LastWriteTime;
		ULONG      TitleIndex, NameLength;
		wchar_t    Name[max_length + 1];
	};

	extern "C"
	{
		typedef NTSTATUS(__stdcall RTLFORMATCURRENTUSERKEYPATH)(UNICODE_STRING*);
		typedef NTSTATUS(__stdcall NTCREATEKEY)(HANDLE*, ULONG DesiredAccess, OBJECT_ATTRIBUTES*, ULONG, UNICODE_STRING* Class, ULONG CreateOptions, ULONG* Disposition);
		typedef NTSTATUS(__stdcall NTOPENKEY)(HANDLE*, ULONG DesiredAccess, OBJECT_ATTRIBUTES*);
		typedef NTSTATUS(__stdcall NTENUMERATEKEY)(HANDLE, ULONG Index, KEY_INFORMATION_CLASS, void* KeyInformation, ULONG KeyInformationLength, ULONG* ResultLength);
		typedef NTSTATUS(__stdcall NTENUMERATEVALUEKEY)(HANDLE, ULONG Index, KEY_VALUE_INFORMATION_CLASS, void* KeyValueInformation, ULONG KeyValueInformationLength, ULONG* ResultLength);
		typedef NTSTATUS(__stdcall NTQUERYVALUEKEY)(HANDLE, UNICODE_STRING* ValueName, KEY_VALUE_INFORMATION_CLASS, void* KeyValueInformation, ULONG Length, ULONG* ResultLength);
		typedef NTSTATUS(__stdcall NTSETVALUEKEY)(HANDLE, UNICODE_STRING* ValueName, ULONG TitleIndex, ULONG Type, void* Data, ULONG DataSize);
		typedef NTSTATUS(__stdcall NTDELETEVALUEKEY)(HANDLE, UNICODE_STRING* ValueName);
		typedef NTSTATUS(__stdcall NTDELETEKEY)(HANDLE);
		typedef NTSTATUS(__stdcall NTCLOSE)(HANDLE);
	}

	static RTLFORMATCURRENTUSERKEYPATH* RtlFormatCurrentUserKeyPath;
	static NTCREATEKEY*          NtCreateKey;
	static NTOPENKEY*            NtOpenKey;
	static NTENUMERATEKEY*       NtEnumerateKey;
	static NTENUMERATEVALUEKEY*  NtEnumerateValueKey;
	static NTQUERYVALUEKEY*      NtQueryValueKey;
	static NTSETVALUEKEY*        NtSetValueKey;
	static NTDELETEVALUEKEY*     NtDeleteValueKey;
	static NTDELETEKEY*          NtDeleteKey;
	static NTCLOSE*              NtClose;

	class NtDllScopedLoader
	{
		HMODULE hNtDll;
	public:
		NtDllScopedLoader()
		{
			hNtDll = LoadLibraryA("ntdll.dll");
			if (!hNtDll)
			{
				std::wcout << L"LoadLibraryA failed loading ntdll.dll\n";
				return;
			}
			RtlFormatCurrentUserKeyPath = (RTLFORMATCURRENTUSERKEYPATH*)GetProcAddress(hNtDll, "RtlFormatCurrentUserKeyPath");
			NtCreateKey = (NTCREATEKEY*)GetProcAddress(hNtDll, "NtCreateKey");
			NtOpenKey = (NTOPENKEY*)GetProcAddress(hNtDll, "NtOpenKey");
			NtEnumerateKey = (NTENUMERATEKEY*)GetProcAddress(hNtDll, "NtEnumerateKey");
			NtEnumerateValueKey = (NTENUMERATEVALUEKEY*)GetProcAddress(hNtDll, "NtEnumerateValueKey");
			NtQueryValueKey = (NTQUERYVALUEKEY*)GetProcAddress(hNtDll, "NtQueryValueKey");
			NtSetValueKey = (NTSETVALUEKEY*)GetProcAddress(hNtDll, "NtSetValueKey");
			NtDeleteValueKey = (NTDELETEVALUEKEY*)GetProcAddress(hNtDll, "NtDeleteValueKey");
			NtDeleteKey = (NTDELETEKEY*)GetProcAddress(hNtDll, "NtDeleteKey");
			NtClose = (NTCLOSE*)GetProcAddress(hNtDll, "NtClose");
		}
		~NtDllScopedLoader()
		{
			if (hNtDll) FreeLibrary(hNtDll);
		}
	};
	static const NtDllScopedLoader static_ntdll_loader;
}

namespace nt_cpp
{
	typedef HANDLE           HANDLE;
	typedef ULONG            ULONG, Tt;
	typedef USHORT           USHORT;
	typedef DWORD            DWORD;
	typedef std::string          string;
	typedef std::wstring         wstring;
	typedef wstring::size_type   St;
	class Udc
	{
		enum
		{
			None = 0, Str = 1, StrEx = 2, Bin = 3, Dw = 4
		};
		nt::UNICODE_STRING ucstr;
		wstring buf;
		void SyUsSz(St size)
		{
			ucstr.Buffer = &buf[0]; ucstr.Length = USHORT(size); ucstr.MaximumLength = USHORT(buf.length() * 2);
		}
		void SyUs(St len)
		{
			SyUsSz(len * 2);
		}
		static void Cpy(void*d, const void*s, St sz)
		{
			for (St i = 0; i < sz; ++i)
			{
				*((char*)d + i) = *((const char*)s + i);
			}
		}
		void unalgn_asgn(const void*s, St sz)
		{
			buf.assign((sz + 3) / 2, L'\0'); Cpy(&buf[0], s, sz); SyUsSz(sz);
		}
		static wstring hex(char b)
		{
			static const wchar_t hd[] = L"0123456789abcdef";
			return wstring(1, hd[(b >> 4) & 15]) + hd[b & 15];
		}
	public:
		ULONG type;
		const static St npos = ~St(0);
		St size() const
		{
			return ucstr.Length;
		}
		St length() const
		{
			return size() / 2;
		}
		bool empty() const
		{
			return !size();
		}
		void* data()
		{
			return ucstr.Buffer;
		}
		const void* data() const
		{
			return ucstr.Buffer;
		}
		Udc(St reserve = 0) : buf(reserve + 1, L'\0'), type(reserve ? Str : None)
		{
			SyUs(0);
		}
		Udc(const Udc& s) : type(s.type)
		{
			unalgn_asgn(s.data(), s.size());
		}
		Udc(const wstring& ws) : buf(ws + L'\0'), type(Str)
		{
			SyUs(ws.length());
		}
		template <St ct> Udc(const wchar_t(&s)[ct]) : buf(s, ct), type(Str)
		{
			SyUs(ct - !s[ct - 1]);
		}
		Udc(const void* s, St sz, ULONG tp = Bin) : buf((sz + 3) / 2, L'\0'), type(tp)
		{
			SyUsSz(sz); Cpy(data(), s, sz);
		}
		Udc(const wchar_t*s, St len) : buf(s, len), type(Str)
		{
			buf += L'\0'; SyUs(len);
		}
		Udc(St len, wchar_t wc) : buf(len + 1, wc), type(Str)
		{
			buf[len] = 0; SyUs(len);
		}
		void pop_back()
		{
			if (ucstr.Length >= 2) ucstr.Length -= 2;
		}
		wchar_t back() const
		{
			return length() ? ucstr.Buffer[length() - 1] : L'\0';
		}
		wchar_t* begin()
		{
			return ucstr.Buffer;
		}
		wchar_t* end()
		{
			return begin() + length();
		}
		const wchar_t* begin() const
		{
			return ucstr.Buffer;
		}
		const wchar_t* end()   const
		{
			return begin() + length();
		}
		Udc& operator = (Udc s)
		{
			type = s.type; unalgn_asgn(s.data(), s.size()); return *this;
		}
		operator nt::UNICODE_STRING* ()
		{
			return &ucstr;
		}
		operator void* ()
		{
			return data();
		}
		operator wstring () const
		{
			switch (type)
			{
				case Str: case StrEx:
				{
					wstring ws(length(), L'\0');
					Cpy(&ws[0], data(), length() * 2);
					return ws;
				}
				case Bin:
				{
					const char* p = (const char*)data();
					wstring ws;
					for (St i = 0; i < size(); ++i)
					{
						if (i)ws += L' '; ws += hex(p[i]);
					}
					return ws;
				}
				case Dw:
				{
					if (size() < 4) return wstring();
					const char* p = (const char*)data();
					return L"0x" + hex(p[3]) + hex(p[2]) + hex(p[1]) + hex(p[0]);
				}
				case None: default: return L"REG_NONE";
			}
		}
		friend std::wostream& operator << (std::wostream& os, const Udc& udc)
		{
			return os << wstring(udc);
		}
		Udc& operator += (const Udc& s)
		{
			if (s.type == None) return *this;
			if (type == None) return *this = s;
			buf = wstring(*this) + wstring(s) + L'\0';
			type = Str;
			SyUs(buf.length() - 1);
			return *this;
		}
		Udc& operator += (wchar_t wc)
		{
			return operator += (Udc(1, wc));
		}
		friend Udc operator + (Udc a, const Udc &b)
		{
			return a += b;
		}
		friend Udc operator + (wchar_t a, const Udc& b)
		{
			Udc r(1, a); return r += b;
		}
		friend Udc operator + (Udc a, wchar_t b)
		{
			return a += b;
		}
		template <St ct> friend Udc operator + (Udc a, const wchar_t(&s)[ct])
		{
			return a += Udc(s);
		}
		template <St ct> friend Udc operator + (const wchar_t(&s)[ct], const Udc& b)
		{
			Udc a(s); return a += b;
		}
		DWORD dword() const
		{
			return size() == 4 ? *(const DWORD*)data() : 0;
		}
		St find(wchar_t wc) const
		{
			return wstring(*this).find(wc);
		}
		St rfind(wchar_t wc) const
		{
			return wstring(*this).rfind(wc);
		}
		Udc substr(St start, St len = npos) const
		{
			return wstring(*this).substr(start, len);
		}
	};

	inline std::pair<Udc, Udc> SplitFullPath(Udc full_path)
	{
		const auto delim = full_path.rfind(L'\\');
		if (delim == Udc::npos) return std::pair<Udc, Udc>(full_path, Udc());
		return std::pair<Udc, Udc>(full_path.substr(0, delim), full_path.substr(delim + 1));
	}

	static Udc GetCurrentUserPath()
	{
		Udc path(512);
		nt::RtlFormatCurrentUserKeyPath(path);
		return path;
	}
	static HANDLE OpenKey(Udc key_path, ULONG desired_access)
	{
		while (key_path.back() == L'\\') key_path.pop_back();
		nt::OBJECT_ATTRIBUTES path;
		path.ObjectName = key_path;
		HANDLE hkey = 0;
		return !nt::NtOpenKey(&hkey, desired_access, &path) ? hkey : 0;
	}
	static void CloseKey(HANDLE hkey)
	{
		nt::NtClose(hkey);
	}
	static HANDLE CreateKey(Udc key_path, ULONG desired_access = KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_ENUMERATE_SUB_KEYS, ULONG create_options = REG_OPTION_NON_VOLATILE)
	{
		using namespace nt;
		while (key_path.back() == L'\\')
		{
			key_path.pop_back();
		}
		OBJECT_ATTRIBUTES path;
		path.ObjectName = key_path;
		ULONG disposition = 0;
		for (int i = 0; i < 2; ++i)
		{
			HANDLE hkey = 0;
			NTSTATUS status = NtCreateKey(&hkey, desired_access, &path, 0, 0, create_options, &disposition);
			if (!status) return hkey;
			if (i || status != STATUS_OBJECT_NAME_NOT_FOUND) return 0;
			auto i_path_up = key_path.rfind(L'\\');
			if (i_path_up == Udc::npos) return 0;
			Udc path_up = key_path.substr(0, i_path_up);
			hkey = CreateKey(path_up, desired_access, create_options);
			if (!hkey) return 0;
			CloseKey(hkey);
		}
		return 0;
	}
	static Udc QueryValue(HANDLE hkey, Udc value_name)
	{
		using namespace nt;
		KEY_VALUE_FULL_INFORMATION<2048> vi; // TODO: allow arbitrary size
		ULONG result_size = 0;
		NTSTATUS status = NtQueryValueKey(hkey, value_name, KeyValueFullInformation, &vi, sizeof(vi), &result_size);
		if (status) return Udc();
		return Udc(vi.GetData(), vi.DataLength, vi.Type);
	}
	static Udc QueryValue(Udc full_path, ULONG bitness_flag = 0)
	{
		auto key_value_name = SplitFullPath(full_path);
		HANDLE hkey = OpenKey(key_value_name.first, KEY_QUERY_VALUE | bitness_flag);
		if (!hkey) return Udc();
		const Udc value = QueryValue(hkey, key_value_name.second);
		CloseKey(hkey);
		return value;
	}
	static bool SetValue(HANDLE hkey, Udc value_name, Udc data)
	{
		return !nt::NtSetValueKey(hkey, value_name, 0, data.type, data, data.size());
	}
	static bool SetValue(Udc full_path, Udc data, ULONG bitness_flag = 0)
	{
		using namespace nt;
		auto key_value_name = SplitFullPath(full_path);
		HANDLE hkey = OpenKey(key_value_name.first, KEY_SET_VALUE | bitness_flag);
		if (!hkey) return false;
		bool success = SetValue(hkey, key_value_name.second, data);
		CloseKey(hkey);
		return success;
	}
	static bool DeleteValue(HANDLE hkey, Udc value_name)
	{
		return !nt::NtDeleteValueKey(hkey, value_name);
	}
	static bool DeleteValue(Udc full_path, ULONG bitness_flag = 0)
	{
		auto key_value_name = SplitFullPath(full_path);
		HANDLE hkey = OpenKey(key_value_name.first, KEY_SET_VALUE | bitness_flag);
		if (!hkey) return false;
		bool success = DeleteValue(hkey, key_value_name.second);
		CloseKey(hkey);
		return success;
	}
	static bool DeleteKey(HANDLE hkey)
	{
		return !nt::NtDeleteKey(hkey);
	}
	static bool DeleteKey(Udc key_path, ULONG bitness_flag = 0)
	{
		HANDLE hkey = OpenKey(key_path, DELETE | bitness_flag);
		if (!hkey) return false;
		bool success = DeleteKey(hkey);
		CloseKey(hkey);
		return success;
	}
}