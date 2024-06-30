
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ConfigFile
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Parse '-', 'stdin' and '1' as STDIN, or open a regular text file in UTF8 read mode.
	//
	// An error line is printed via `tprintf()` when the given path turns out not to be valid.
	StdioConfigReader::StdioConfigReader(const char *path)
		: _f(nullptr)
	{
		if (!path || !*path) {
			return;
		}

		if (strieq(path, "/dev/stdin") || strieq(path, "stdin") || strieq(path, "-") || strieq(path, "1"))
			_f = stdin;
		else {
			fs::path p = fs::weakly_canonical(path);
			std::u8string p8 = p.u8string();
			std::string ps = reinterpret_cast<const char *>(p8.c_str());
			_f = fopen(ps.c_str(), "r");
			if (!_f) {
				PARAM_ERROR("Cannot open file for reading its content: {}\n", ps);
			}
		}
	}

	StdioConfigReader::StdioConfigReader(const std::string &path)
		: StdioConfigReader(path.c_str())
	{}

	StdioConfigReader::~StdioConfigReader() {
		if (_f) {
			if (_f != stdin) {
				fclose(_f);
			} else {
				fflush(_f);
			}
		}
	}

	FILE *StdioConfigReader::operator()() const {
		return _f;
	}

	bool StdioConfigReader::ReadInfoLine(ConfigReader::line &line) {
		line.init();
		CString<1024> buf;
	get_next_line:
		buf.clear();
		for (;;) {
			// when we're looping, the next line *appends* the freshly read content into `buf`, thanks to the `shift_start()` call at the end below!
			char *d = buf.data();
			char *p = fgets(d, buf.datasize(), _f);
			if (!p) {
				// when error is signaled, the data is undetermined: NIL it then!
				if (ferror(_f)) {
					*d = 0;
					line.error = true;
				}
				// EOF reached? If we got *anything*, that'll be the last line in the file.
				line.EOF_reached = feof(_f);
				if (buf.length() > 0)
					_lineno++;
				break;
			}
			if (buf.endsWithAny("\r\n")) {
				_lineno++;
				break;
			}
			// incoming line is larger than expected: enlarge the buffer.
			buf.shift_start(buf.length());
			buf.resize(buf.allocsize());
		}
		buf.reset_start();
		buf.Trim();
		// did we hit an empty line?
		if (buf.empty()) {
			goto get_next_line;
		}
		// did we hit a comment line?
		if (buf.startsWithAny("#;")) {
			goto get_next_line;
		}
		if (buf.startsWith("//")) {
			goto get_next_line;
		}
		// we found an actual content line: produce it
		line.content = buf.c_str();
		line.linenumber = _lineno;
		return !((line.EOF_reached && line.content.empty()) || line.error);
	}











	StringConfigReader::StringConfigReader(const char *data, size_t size) {
		LIBASSERT_ASSERT(data != nullptr);
		_buffer.assign(data, size);
	}

	StringConfigReader::StringConfigReader(const uint8_t *data, size_t size) {
		LIBASSERT_ASSERT(data != nullptr);
		_buffer.assign(data, size);
	}

	StringConfigReader::StringConfigReader(const char *data_string) {
		LIBASSERT_ASSERT(data_string != nullptr);
		_buffer.assign(data_string);
	}

	StringConfigReader::StringConfigReader(const std::string &data) {
		_buffer.assign(data);
	}

	StringConfigReader::~StringConfigReader() {
		_buffer.clear();
	}

	bool StringConfigReader::ReadInfoLine(ConfigReader::line &line) {
		line.init();
		if (_buffer.empty()) {
			// EOF reached. Nothing availabel.
			line.EOF_reached = true;
		} else {
			for (;;) {
				_lineno += _buffer.TrimLeftCountingNewlines();

				// did we hit the end of the buffer?
				if (_buffer.empty()) {
					line.EOF_reached = true;
					break;
				}

				char *s = _buffer.data();
				size_t offset = strcspn(s, "\r\n");

				_lineno++;
				line.linenumber = _lineno;

				char *e = s + offset;
				// count the pack of consecutive newlines and skip 'em on the next round through here.
				unsigned int lf_count = 0;
				while (*e && isspace(*e)) {
					if (*e == '\n')
						lf_count++;
					e++;
				}
				if (lf_count > 1)
					_lineno += lf_count - 1;

				// skip embedded NUL bytes, if there are any:
				char *stop = s + _buffer.datasize();
				while (!*e && stop > e)
					e++;

				// this is the amount we need to jump over for our next round through here to be properly positioned after the newline series above.
				//
				// either way, this shift will jump over the NUL sentinel we're about to inject just below.
				size_t shift = (e - s);

				s[offset] = 0;

				// did we NOT hit a comment line? Nor an empty line?
				if (!_buffer.startsWithAny("#;") && !_buffer.startsWith("//")) {
					// trim trailing whitespace at the tail end of the line.
					e = s + offset;
					e--;
					while (e >= s && isspace(*e))
						e--;
					e++;
					*e = 0;
					// leading whitespace has already been trimmed at the start of the loop, so we're golden now to check for an empty line.
					if (*s) {
						// yay! Got a real live one: feed it to the caller!
						line.content = std::string_view(s, e - s);

						_buffer.shift_start(shift);

						// early notification about EOF:
						line.EOF_reached = _buffer.empty();
						break;
					}
				}
				_buffer.shift_start(shift);
			}
		}
		return !line.EOF_reached;
	}



}  // namespace
