
#ifndef _LIB_PARAMS_SNAPSHOTS_H_
#define _LIB_PARAMS_SNAPSHOTS_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <variant>


namespace parameters {

	class Param;
	class ParamsVectorSet;

	class Snapshot;
	class SnapshotSeries;

	typedef Snapshot *SnapshotPtr;

	class Snapshot {
		friend class SnapshotSeries;

	protected:
		Snapshot(const char *name = nullptr);
		~Snapshot() = default;

		static SnapshotPtr TakeSnapshot(const char *name, ParamsVectorSet *globals, ParamsVectorSet *locals);
		static void ResetToSnapshot(Snapshot &snapshot);

	protected:
		std::string name;

		struct datums {
			Param *param_ref;
			std::variant<int32_t, bool, double, std::string> value;
			Param::access_counts_t stats;
		};

		std::vector<datums> data;
	};

	class SnapshotSeries {
	protected:
		SnapshotSeries() = default;
		~SnapshotSeries();

	public:
		Snapshot &TakeSnapshot(const char *name, ParamsVectorSet *globals, ParamsVectorSet *locals);
		void RewindToSnapshot(Snapshot &snapshot);
		void PopSnapshot();

	protected:
		std::vector<SnapshotPtr> series;
	};

}	// namespace

#endif
