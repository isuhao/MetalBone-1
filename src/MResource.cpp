#include "MResource.h"
#include "MEvent.h"
#include "3rd/XUnzip.h"
namespace MetalBone
{
	// ========== MFont ==========
	struct MFontData
	{
		enum FontStyle
		{
			None      = 0,
			Bold      = 0x1,
			Italic    = 0x2
		};
		inline MFontData();
		inline ~MFontData();

		HFONT fontHandle;
		int refCount;
		unsigned int ptSize;
		unsigned int style;
		
		std::wstring faceName;

		typedef std::tr1::unordered_multimap<std::wstring, MFont> FontCache;
		static FontCache cache;
	};
	inline MFontData::MFontData():fontHandle(NULL),refCount(0),style(None),ptSize(9){}
	inline MFontData::~MFontData(){ if(fontHandle != NULL) ::DeleteObject(fontHandle); }
	MFontData::FontCache MFontData::cache;
	
	MFont::~MFont()
	{
		--data->refCount;
		if(data->refCount == 0)
			delete data;
	}

	MFont::MFont()
	{
		static bool inited = false;
		static MFontData defaultData;
		if(inited == false)
		{
			LOGFONTW lf;
			memset(&lf,0,sizeof(LOGFONTW));
			lf.lfHeight = mApp->winDpi() / 72 * 9;
			lf.lfWeight = FW_NORMAL;
			lf.lfQuality = ANTIALIASED_QUALITY;
			wcscpy_s(lf.lfFaceName,L"Arial");
			defaultData.fontHandle = ::CreateFontIndirectW(&lf);
			defaultData.refCount = 1;
			defaultData.faceName = L"Arial";
			inited = true;
		}

		data = &defaultData;
		++defaultData.refCount;
	}

	MFont::MFont(const LOGFONTW& lf)
	{
		std::wstring faceName(lf.lfFaceName);
		unsigned int style = lf.lfWeight == FW_BOLD ? MFontData::Bold : MFontData::None;
		if(lf.lfItalic) style |= MFontData::Italic;
		unsigned int ptSize = lf.lfHeight * 72 / mApp->winDpi();

		std::pair<MFontData::FontCache::iterator, MFontData::FontCache::iterator>
			cIts = MFontData::cache.equal_range(faceName);
		MFontData::FontCache::iterator& it = cIts.first;
		while(it != cIts.second)
		{
			MFontData* d = it->second.data;
			if(d->style == style && d->ptSize == ptSize)
			{
				data = d;
				++data->refCount;
				return;
			}
			++it;
		}

		data = new MFontData();
		data->fontHandle = ::CreateFontIndirectW(&lf);
		data->refCount   = 1;
		data->style      = style;
		data->ptSize     = ptSize;
		data->faceName   = faceName;
		MFontData::cache.insert(MFontData::FontCache::value_type(faceName,*this));
	}

	MFont::MFont(const MFont& rhs)
	{
		data = rhs.data;
		++data->refCount;
	}

	MFont::MFont(unsigned int size, const std::wstring& faceName,
		bool bold, bool italic, bool pixelUnit)
	{
		// Search for sharing.
		unsigned int style = bold ? MFontData::Bold : MFontData::None;
		if(italic) style |= MFontData::Italic;
		unsigned int ptSize = pixelUnit ? size * mApp->winDpi() / 72 : size;

		std::pair<MFontData::FontCache::iterator, MFontData::FontCache::iterator>
			cIts = MFontData::cache.equal_range(faceName);
		MFontData::FontCache::iterator& it = cIts.first;
		while(it != cIts.second)
		{
			MFontData* d = it->second.data;
			if(d->style == style && d->ptSize == ptSize)
			{
				data = d;
				++data->refCount;
				return;
			}
			++it;
		}

		LOGFONTW lf;
		memset(&lf,0,sizeof(LOGFONTW));
		lf.lfHeight = 0 - (pixelUnit ? size : size * mApp->winDpi() / 72);
		lf.lfWeight = FW_NORMAL;
		lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
		lf.lfItalic = italic ? TRUE : FALSE;
		lf.lfQuality = ANTIALIASED_QUALITY;
		lf.lfPitchAndFamily = DEFAULT_PITCH;
		wcscpy_s(lf.lfFaceName, faceName.c_str());

		data = new MFontData();
		data->fontHandle = ::CreateFontIndirectW(&lf);
		data->faceName   = faceName;
		data->refCount   = 1;
		data->style      = style;
		data->ptSize     = ptSize;

		MFontData::cache.insert(MFontData::FontCache::value_type(faceName,*this));
	}

	HFONT MFont::getHandle()
		{ return data->fontHandle; }
	bool MFont::isBold() const
		{ return (data->style & MFontData::Bold)   != 0; }
	bool MFont::isItalic() const
		{ return (data->style & MFontData::Italic) != 0; }
	const std::wstring& MFont::getFaceName() const
		{ return data->faceName; }
	unsigned int MFont::pointSize() const
		{ return data->ptSize; }

	const MFont& MFont::operator=(const MFont& rhs)
	{
		--data->refCount;
		if(data->refCount == 0)
			delete data;
		data = rhs.data;
		++data->refCount;
		return *this;
	}

	MFont::operator HFONT() const
		{ return data->fontHandle; }





	// ========== MCursor ==========
	MCursor* MCursor::appCursor = 0;
	const MCursor* MCursor::showingCursor = 0;
	void MCursor::setType(CursorType t)
	{
		M_ASSERT(t != CustomCursor);
		if(e_type == t)
			return;

		HCURSOR newHandle = NULL;
		if(t == BlankCursor)
		{
			BYTE andMask[] = { 0xFF };
			BYTE xorMask[] = { 0 };
			newHandle = ::CreateCursor(mApp->getAppHandle(),0,0,8,1,andMask,xorMask);
		} else {
			LPWSTR c;
			switch(t)
			{
				case ArrowCursor:       c = IDC_ARROW;       break;
				case CrossCursor:       c = IDC_CROSS;       break;
				case HandCursor:        c = IDC_HAND;        break;
				case HelpCursor:        c = IDC_HELP;        break;
				case IBeamCursor:       c = IDC_IBEAM;       break;
				case WaitCursor:        c = IDC_WAIT;        break;
				case SizeAllCursor:     c = IDC_SIZEALL;     break;
				case SizeVerCursor:     c = IDC_SIZENS;      break;
				case SizeHorCursor:     c = IDC_SIZEWE;      break;
				case SizeBDiagCursor:   c = IDC_SIZENESW;    break;
				case SizeFDiagCursor:   c = IDC_SIZENWSE;    break;
				case AppStartingCursor: c = IDC_APPSTARTING; break;
				case ForbiddenCursor:   c = IDC_NO;          break;
				case UpArrowCursor:     c = IDC_UPARROW;     break;
			}
			newHandle = ::LoadCursorW(NULL, c);
		}

		updateCursor(newHandle, t);
	}

	void MCursor::updateCursor(HCURSOR h, CursorType t)
	{
		if(h == NULL)
		{
			h = ::LoadCursorW(NULL,IDC_ARROW);
			t = ArrowCursor; 
		}

		HCURSOR oldCursor = handle;
		if(showingCursor == this)
			::SetCursor(h);
		if(e_type == BlankCursor || e_type == CustomCursor)
			::DestroyCursor(oldCursor);

		handle = h;
		e_type = t;
	}

	MCursor::~MCursor()
	{
		if(showingCursor == this)
		{
			showingCursor = 0;
			if(e_type != ArrowCursor)
				::SetCursor(::LoadCursorW(NULL,IDC_ARROW));
		}

		if(e_type == BlankCursor || e_type == CustomCursor)
			::DestroyCursor(handle);
	}

	void MCursor::show(bool forceUpdate) const
	{
		if(appCursor != 0)
			return;

		if(!forceUpdate)
		{
			if(showingCursor == this)
				return;

			if(showingCursor != 0)
			{
				if(showingCursor->e_type != CustomCursor)
				{
					if(showingCursor->e_type == e_type)
						return;
				} else if(showingCursor->handle == handle)
					return;
			}
		}

		showingCursor = this;
		::SetCursor(handle);
	}





	// ========== MTimer ==========
	MTimer::TimerHash MTimer::timerHash;
	MTimer::TimerIdMap MTimer::timerIdMap;
	void MTimer::start()
	{
		if(b_active) return;
		b_active = true;
		TimerHash::iterator it = timerHash.find(m_interval);
		TimerHash::iterator itEnd = timerHash.end();
		if(it == itEnd) {
			m_id = SetTimer(NULL,0,m_interval,timerProc);
			timerIdMap.insert(TimerIdMap::value_type(m_id,m_interval));
		} else { m_id = it->second->m_id; }
		timerHash.insert(TimerHash::value_type(m_interval,this));
	}
	void MTimer::stop()
	{
		if(!b_active) return;
		b_active = false;
		TimerHash::iterator it = timerHash.find(m_interval);
		TimerHash::iterator itEnd = timerHash.end();
		while(it != itEnd) { if(it->second == this) break; }
		timerHash.erase(it);
		it = timerHash.find(m_interval);
		if(it == itEnd) { 
			timerIdMap.erase(m_id);
			KillTimer(NULL,m_id);
		}
		m_id = 0;
	}

	void __stdcall MTimer::timerProc(HWND, UINT, UINT_PTR id, DWORD)
	{
		TimerIdMap::iterator it = timerIdMap.find(id);
		TimerIdMap::iterator itEnd = timerIdMap.end();
		if(it == itEnd)
			return;
		unsigned int interval = it->second;
		std::pair<TimerHash::iterator,TimerHash::iterator> range =
			timerHash.equal_range(interval);
		while(range.first != range.second)
		{
			MTimer* timer = range.first->second;
			if(timer->isSingleShot()) {
				timer->stop();
				range = timerHash.equal_range(interval);
			} else
				++range.first;
			timer->timeout();
		}
	}

	void MTimer::setInterval(unsigned int msec)
	{
		m_interval = msec;
		if(!b_active)
			return;
		stop();
		start();
	}

	void MTimer::cleanUp()
	{
		TimerIdMap::iterator it = timerIdMap.begin();
		TimerIdMap::iterator itEnd = timerIdMap.end();
		while(it != itEnd)
		{
			KillTimer(NULL,it->first);
			++it;
		}
		timerIdMap.clear();
		TimerHash::iterator tit = timerHash.begin();
		TimerHash::iterator titEnd = timerHash.end();
		while(tit != titEnd)
		{
			tit->second->b_active = false;
			tit->second->m_id = 0;
			++tit;
		}
		timerHash.clear();
	}
	struct MTimerCleanUpHelper { ~MTimerCleanUpHelper() { MTimer::cleanUp(); } };
	MTimerCleanUpHelper timerCleanUpHelper;





	// ========== MShortCut ==========
	MShortCut::ShortCutMap MShortCut::scMap;
	inline void MShortCut::removeFromMap()
	{
		std::pair<ShortCutMap::iterator, ShortCutMap::iterator> range = scMap.equal_range(getKey());
		ShortCutMap::iterator it = range.first;
		while(it != range.second)
		{
			if(it->second == this)
			{
				scMap.erase(it);
				break;
			}
			++it;
		}
	}
	void MShortCut::setGlobal(bool on)
	{
		if(global == on)
			return;
		global = on;

		if(global)
		{
			target = 0;
			if(modifier == 0 || virtualKey == 0)
				return;
			unsigned int mod = modifier & (~KeypadModifier) | MOD_NOREPEAT;
			::RegisterHotKey(0,getKey(),mod,virtualKey);
		} else {
			::UnregisterHotKey(0,getKey());
		}
	}
	void MShortCut::setKey(unsigned int m, unsigned int v)
	{
		if(m == modifier && v == virtualKey)
			return;
		enabled = true;
		if(global)
			::UnregisterHotKey(0,getKey());
		removeFromMap();
		modifier = m;
		virtualKey = v;
		if(global) {
			global = false;
			setGlobal(true);
		}
		insertToMap();
	}

	std::vector<MShortCut*> MShortCut::getMachedShortCuts(unsigned int modifier, unsigned int virtualKey)
	{
		std::vector<MShortCut*> scs;
		unsigned int key = (modifier << 26) | virtualKey;
		std::pair<ShortCutMap::iterator, ShortCutMap::iterator> range = scMap.equal_range(key);
		ShortCutMap::iterator it = range.first;
		while(it != range.second)
		{
			if(it->second->isEnabled())
				scs.push_back(it->second);
			++it;
		}

		return scs;
	}





	// ========== MResource ==========
	MResource::ResourceCache MResource::cache;
	bool MResource::open(HINSTANCE hInstance, const wchar_t* name, const wchar_t* type)
	{
		HRSRC hrsrc = ::FindResourceW(hInstance,name,type);
		if(hrsrc) {
			HGLOBAL hglobal = ::LoadResource(hInstance,hrsrc);
			if(hglobal)
			{
				buffer = (const BYTE*)::LockResource(hglobal);
				if(buffer) {
					size = ::SizeofResource(hInstance,hrsrc);
					return true;
				}
			}
		}
		return false;
	}

	bool MResource::open(const std::wstring& fileName)
	{
		ResourceCache::const_iterator iter;
		if(fileName.at(0) == L':' && fileName.at(1) == L'/')
		{
			std::wstring temp(fileName,2,fileName.length() - 2);
			iter = cache.find(temp);
		} else {
			iter = cache.find(fileName);
		}
		if(iter == cache.cend())
			return false;

		buffer = iter->second->buffer;
		size   = iter->second->length;
		return true;
	}

	void MResource::clearCache()
	{
		ResourceCache::const_iterator it    = cache.cbegin();
		ResourceCache::const_iterator itEnd = cache.cend();
		while(it != itEnd) { delete it->second; ++it; }
		cache.clear();
	}

	bool MResource::addFileToCache(const std::wstring& filePath)
	{
		if(cache.find(filePath) != cache.cend())
			return false;

		HANDLE resFile = ::CreateFileW(filePath.c_str(),GENERIC_READ,
			FILE_SHARE_READ,0,OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,0);
		if(resFile == INVALID_HANDLE_VALUE)
			return false;

		ResourceEntry* newEntry = new ResourceEntry();

		DWORD size = ::GetFileSize(resFile,0);
		newEntry->length = size;
		newEntry->unity  = true;
		newEntry->buffer = new BYTE[size];

		DWORD readSize;
		if(::ReadFile(resFile,newEntry->buffer,size,&readSize,0))
		{
			if(readSize != size) {
				delete newEntry;
			} else {
				cache.insert(ResourceCache::value_type(filePath, newEntry));
				return true;
			}
		} else {
			delete newEntry;
		}
		return false;
	}

	bool MResource::addZipToCache(const std::wstring& zipPath)
	{
		HZIP zip = OpenZip((void*)zipPath.c_str(), 0, ZIP_FILENAME);
		if(zip == 0)
			return false;

		bool success = true;
		ZIPENTRYW zipEntry;
		if(ZR_OK == GetZipItem(zip, -1, &zipEntry))
		{
			std::vector<std::pair<int,ResourceEntry*> > tempCache;

			int totalItemCount = zipEntry.index;
			int buffer = 0;
			// Get the size of every item.
			for(int i = 0; i < totalItemCount; ++i)
			{
				GetZipItem(zip,i,&zipEntry);
				if(!(zipEntry.attr & FILE_ATTRIBUTE_DIRECTORY)) // ignore every folder
				{
					ResourceEntry* entry = new ResourceEntry();
					entry->buffer += buffer;
					entry->length = zipEntry.unc_size;

					cache.insert(ResourceCache::value_type(zipEntry.name,entry));
					tempCache.push_back(std::make_pair(i,entry));

					buffer += zipEntry.unc_size;
				}
			}

			BYTE* memBlock = new BYTE[buffer]; // buffer is the size of the zip file.

			// Uncompress every item.
			totalItemCount = tempCache.size();
			for(int i = 0; i < totalItemCount; ++i)
			{
				ResourceEntry* re = tempCache.at(i).second;
				re->buffer = memBlock + (int)re->buffer;

				UnzipItem(zip, tempCache.at(i).first,
					re->buffer, re->length, ZIP_MEMORY);
			}

			tempCache.at(0).second->unity = true;
		} else {
			success = false;
		}

		CloseZip(zip);
		return success;
	}
}