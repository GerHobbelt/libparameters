/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_VECTORSETS_H_
#define _LIB_PARAMS_VECTORSETS_H_

#include <parameters/parameter_classes.h>

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>


namespace parameters {

	class Snapshot;

	// Custom equivalent of std::hash<Param> + std::equal_to<Param> for std::unordered_map<const char *key, Param & value>.
	class ParamHash
	{
	public:
		// hash:
		std::size_t operator()(const Param& s) const noexcept;
		std::size_t operator()(const char * s) const noexcept;

		// equal_to:
		bool operator()(const Param& lhs, const Param& rhs) const noexcept;
		// equal_to:
		bool operator()(const char * lhs, const char * rhs) const noexcept;
	};

	// --------------------------------------------------------------------------------------------------

	// Custom implementation of class Compare for std::sort.
	class ParamComparer
	{
	public:
		// compare as a-less-b? for purposes of std::sort et al:
		bool operator()(const Param& lhs, const Param& rhs) const;
		// compare as a-less-b? for purposes of std::sort et al:
		bool operator()(const char * lhs, const char * rhs) const;
	};

	// --------------------------------------------------------------------------------------------------

#include <parameters/sourceref_defstart.h>

	// Hash table used internally as a database table, collecting the compiled-in parameter instances.
	// Used to speed up the various `find()` functions, among others.
	typedef std::unordered_map<
		const char * /* key */,
		ParamPtr /* value */,
		ParamHash /* hash value calc */,
		ParamHash /* equality check */
	> ParamsHashTableType;

	// --------------------------------------------------------------------------------------------------

	// A set (vector) of parameters. While this is named *vector* the internal organization
	// is hash table based to provide fast random-access add / remove / find functionality.
	class ParamsVector {
	protected:
		ParamsHashTableType params_;
		bool is_params_owner_ = false;
		std::string title_;

	public:
		ParamsVector() = delete;
		ParamsVector(const char* title);
		ParamsVector(const char *title, std::initializer_list<ParamPtr> vecs);

		~ParamsVector();

		// **Warning**: this method may only safely be invoked *before* any parameters have been added to the set.
		//
		// Signal the ParamVector internals that this particular set is *parameter instances owner*, i.e. all parameters added to
		// this set also transfer their ownership alongside: while the usual/regular ParamVector only stores *references*
		// to parameter instances owned by *others*, now we will be owner of those parameter instances and thus responsible for heap memory cleanup
		// once our ParamVector destructor is invoked.
		//
		// This feature is used, f.e., in tesseract when the command line parser collects the
		// user-specified parameter values from the command line itself and various referenced or otherwise
		// implicated configuration files: a 'muster set' of known compiled-in parameters is collected and cloned into such
		// an 'owning' parameter vector, which is then passed on to the command line parser proper to use for both parameter name/type/value
		// validations while parsing and storing the parsed values discovered.
		// 
		// It is used to collect, set up and then pass parameter vectors into the tesseract Init* instance
		// methods: by using a (cloned and) *owning* parameter vector, we can simply collect and pass any configuration parameters
		// we wish to have adjusted into the tesseract instance and have these 'activated' only then, i.e. we won't risk
		// modifying any *live* parameters while working on/with the vector-owned set.
		void mark_as_all_params_owner();

		void add(ParamPtr param_ref);
		void add(ParamRef param_ref);
		void add(std::initializer_list<ParamPtr> vecs);

		void remove(ParamPtr param_ref);
		void remove(ParamRef param_ref);
		void remove(const char *name);

		const char* title() const;
		void change_title(const char* title);

		ParamPtr find(
			const char *name,
			ParamType accepted_types_mask
		) const;

		template <ParamDerivativeType T>
		T *find(
			const char *name
		) const;

		std::vector<ParamPtr> as_list(
			ParamType accepted_types_mask = ANY_TYPE_PARAM
		) const;

		friend class ParamsVectorSet;
		friend class Snapshot;
	};

	// --------------------------------------------------------------------------------------------------

	// A set (vector) of surplus parameters, i.e. parameters which are defined at run-time, rather than at compile-time.
	// This SurplusParamsVector class is the owner of each of these (heap allocated) parameters, which are created on demand
	// when calling the add() method.
	class SurplusParamsVector: public ParamsVector {
	public:
		SurplusParamsVector() = delete;
		SurplusParamsVector(const char* title);
		//SurplusParamsVector(const char *title, std::initializer_list<ParamPtr> vecs);

		~SurplusParamsVector();

		void add(const int32_t value, THE_4_HANDLERS_PROTO_4_SURPLUS(IntParam));
		void add(const bool value, THE_4_HANDLERS_PROTO_4_SURPLUS(BoolParam));
		void add(const double value, THE_4_HANDLERS_PROTO_4_SURPLUS(DoubleParam));
		void add(const std::string &value, THE_4_HANDLERS_PROTO_4_SURPLUS(StringParam));

		void add(const std::vector<int32_t> value, const BasicVectorParamParseAssistant &assistant, THE_4_HANDLERS_PROTO_4_SURPLUS(IntSetParam));
		void add(const std::vector<bool> value, const BasicVectorParamParseAssistant &assistant, THE_4_HANDLERS_PROTO_4_SURPLUS(BoolSetParam));
		void add(const std::vector<double> value, const BasicVectorParamParseAssistant &assistant, THE_4_HANDLERS_PROTO_4_SURPLUS(DoubleSetParam));
		void add(const std::vector<std::string> &value, const BasicVectorParamParseAssistant &assistant, THE_4_HANDLERS_PROTO_4_SURPLUS(StringSetParam));

		void add(const char *value, THE_4_HANDLERS_PROTO_4_SURPLUS(StringParam));

	protected:
		void add(ParamPtr param_ref);
		void add(std::initializer_list<ParamPtr> vecs);

	public:
		friend class ParamsVectorSet;
		friend class Snapshot;
	};

	// --------------------------------------------------------------------------------------------------

	// an (ad-hoc?) collection of ParamsVector instances.
	class ParamsVectorSet final {
	private:
		std::vector<ParamsVector *> collection_;

	public:
		ParamsVectorSet();
		ParamsVectorSet(std::initializer_list<ParamsVector *> vecs);

		~ParamsVectorSet();

		void add(ParamsVector &vec_ref);
		void add(ParamsVector *vec_ref);
		void add(std::initializer_list<ParamsVector *> vecs);
		void add(const ParamsVectorSet &vecset_ref);
		void add(const ParamsVectorSet *vecset_ref);

		const std::vector<ParamsVector *> &get() const;

		ParamPtr find(
			const char *name,
			ParamType accepted_types_mask
		) const;

		template <ParamDerivativeType T>
		T *find(
			const char *name
		) const;

		std::vector<ParamPtr> as_list(
			ParamType accepted_types_mask = ANY_TYPE_PARAM
		) const;

		ParamsVector flattened_copy(ParamType accepted_types_mask = ANY_TYPE_PARAM) const;

	public:
		friend class Snapshot;
	};

	// --------------------------------------------------------------------------------------------------

	// template instances:

	// ready-made template instances:
	template <>
	IntParam *ParamsVectorSet::find<IntParam>(
			const char *name) const;
	template <>
	BoolParam *ParamsVectorSet::find<BoolParam>(
			const char *name) const;
	template <>
	DoubleParam *ParamsVectorSet::find<DoubleParam>(
			const char *name) const;
	template <>
	StringParam *ParamsVectorSet::find<StringParam>(
			const char *name) const;
	template <>
	Param *ParamsVectorSet::find<Param>(
			const char *name) const;

	// --------------------------------------------------------------------------------------------------

	// remove the macros to help set up the member prototypes

#include <parameters/sourceref_defend.h>

} // namespace 

#endif
