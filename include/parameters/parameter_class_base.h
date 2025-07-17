/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_CLASSES_BASE_H_
#define _LIB_PARAMS_CLASSES_BASE_H_

#include <parameters/parameter_class_fundamentals.h>
#include <parameters/fmt-support.h>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>


namespace parameters {

#include <parameters/sourceref_defstart.h>

	// --------------------------------------------------------------------------------------------------

	// Definition of various parameter types.
	class Param {
	protected:
		Param(const char *name, const char *comment, ParamsVector &owner, bool init = false);

	public:
		virtual ~Param();

		const char *name_str() const noexcept;
		const char *info_str() const noexcept;
		bool is_init() const noexcept;
		bool is_debug() const noexcept;
		bool is_set() const noexcept;
		bool is_set_to_non_default_value() const noexcept;
		bool is_locked() const noexcept;
		bool has_faulted() const noexcept;

		void lock(bool locking = true);

		bool can_update(ParamSetBySourceType source_type) const noexcept;

		// Signal a (recoverable) fault; used, together with has_faulted() and reset_fault(), by the parameter classes' internals when,
		// f.e., a string value doesn't parse or fails to pass a parameter's validation checks.
		//
		// This signal remains signaled until the next call to reset_fault().
		// DO NOTE that any subsequent parameter value *write* operations for this parameter will internally *reset* the fault state,
		// thus using a clean slate against which the next parse+write (validation+modify checks) will be checked for (new) faults.
		//
		// DO NOTE that each call to `fault()` will bump the error signal count (statistics) for this parameter, hence any 
		// application-level reporting of parameter usage statistics will not miss to report these faults from having occurred.
		void fault() noexcept;

		// Reset the fault state of this parameter, so any subsequent write operation will not be aborted/skipped any more.
		// 
		// Is used to turn the fault signaled state OFF and parse+write a new incoming value.
		void reset_fault() noexcept;

		ParamSetBySourceType set_mode() const noexcept;
		Param *is_set_by() const noexcept;

		ParamsVector &owner() const noexcept;

		// We track Param/Variable setup/changes/usage through this administrative struct.
		// It helps us to diagnose and report which tesseract Params (Variables) are actually
		// USED in which program section and in the program as a whole, while we can also 
		// diagnose which variables have been set up during this session, and by *whom*, as
		// some Params will have been modified due to others having been set, e.g. `debug_all`.
		typedef struct access_counts {
			// the current section's counts
			uint16_t reading;
			uint16_t writing;   // counting the number of *write* actions, answering the question "did we assign a value to this one during this run?"
			uint16_t changing;  // counting the number of times a *write* action resulted in an actual *value change*, answering the question "did we use a non-default value for this one during this run?"
			uint16_t faulting;  // counting the number of times a *parse* action produced a *fault* instead of a legal value to be written into the parameter.
		} access_counts_t;

		const access_counts_t &access_counts() const noexcept;

		// Reset the access count statistics in preparation for the next run.
		// As a side effect the current run's access count statistics will be added to the history
		// set, available via the `prev_sum_*` access_counts_t members.
		void reset_access_counts() noexcept;

		// Fetches the (possibly formatted) value of the param as a string; see the ValueFetchPurpose
		// enum documentation for detailed info which purposes are counted in the access statistics
		// and which aren't.
		virtual std::string value_str(ValueFetchPurpose purpose) const = 0;

		// Fetches the (formatted for print/display) value of the param as a string and does not add 
		// this access to the read counter tally. This is useful, f.e., when printing 'init' 
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process.
		//
		// We do not count this read access as this method is for *display* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string formatted_value_str() const;

		// Fetches the (raw, parseble for re-use via set_value()) value of the param as a string and does not add 
		// this access to the read counter tally. This is useful, f.e., when printing 'init' 
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process. 
		//
		// We do not count this read access as this method is for *validation/comparison* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string raw_value_str() const;

		// Fetches the (formatted for print/display) default value of the param as a string and does not add
		// this access to the read counter tally. This is useful, f.e., when printing 'init'
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process.
		//
		// We do not count this read access as this method is for *display* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string formatted_default_value_str() const;

		// Fetches the (raw, parseble for re-use via set_value()) default value of the param as a string and does not add
		// this access to the read counter tally. This is useful, f.e., when printing 'init'
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process.
		//
		// We do not count this read access as this method is for *validation/comparison* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string raw_default_value_str() const;

		// Return string representing the type of the parameter value, e.g. "integer"
		//
		// We do not count this read access as this method is for *display* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string value_type_str() const;

		// Return string representing the type of the parameter value, e.g. "integer"
		//
		// We do not count this read access as this method is for *print/save-to-file* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string raw_value_type_str() const;

		virtual void set_value(const char *v, SOURCE_REF) = 0;

		// generic:
		void set_value(const std::string &v, SOURCE_REF);

		// return void instead of Param-based return type as we don't accept any copy/move constructors either!
		void operator=(const char *value);
		void operator=(const std::string &value);

		// Optionally the `source_vec` can be used to source the value to reset the parameter to.
		// When no source vector is specified, or when the source vector does not specify this
		// particular parameter, then its value is reset to the default value which was
		// specified earlier in its constructor.
		virtual void ResetToDefault(const ParamsVectorSet *source_vec = nullptr, SOURCE_TYPE) = 0;
		void ResetToDefault(const ParamsVectorSet &source_vec, SOURCE_TYPE);

		Param(const Param &o) = delete;
		Param(Param &&o) = delete;
		Param(const Param &&o) = delete;

		Param &operator=(const Param &other) = delete;
		Param &operator=(Param &&other) = delete;
		Param &operator=(const Param &&other) = delete;

		ParamType type() const noexcept;

	protected:
		const char *name_; // name of this parameter
		const char *info_; // for menus

		Param *setter_;
		ParamsVector &owner_;

#if 0
		ParamValueContainer value_;
		ParamValueContainer default_;
#endif
		mutable access_counts_t access_counts_;

		ParamType type_ : 13;

		ParamSetBySourceType set_mode_ : 4;

		bool init_ : 1; // needs to be set before first use, i.e. can be set 'during application init phase only'
		bool debug_ : 1;
		bool set_ : 1;
		bool set_to_non_default_value_ : 1;
		bool locked_ : 1;
		bool error_ : 1;
	};

	// --------------------------------------------------------------------------------------------------

	// remove the macros to help set up the member prototypes

#include <parameters/sourceref_defend.h>

} // namespace 

#endif
