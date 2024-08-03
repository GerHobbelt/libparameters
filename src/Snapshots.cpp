
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {

	Snapshot::Snapshot(const char *snap_name): name(snap_name) {
	}

	SnapshotPtr Snapshot::TakeSnapshot(const char *name, ParamsVectorSet *globals, ParamsVectorSet *locals) {
		SnapshotPtr snap(new Snapshot());

		size_t items_count = 0;
		if (locals) {
			for (ParamsVector *vec : locals->collection_) {
				items_count += vec->params_.size();
			}
		}
		if (globals) {
			for (ParamsVector *vec : globals->collection_) {
				items_count += vec->params_.size();
			}
		}
		snap->data.reserve(items_count);
		snap->data.resize(items_count);

		size_t item_idx = 0;
		if (locals) {
			for (ParamsVector *vec : locals->collection_) {
				for (auto i : vec->params_) {
					Snapshot::datums &datum = snap->data[item_idx++];
					ParamPtr p = i.second;
					switch (p->type()) {
					case INT_PARAM: {
						IntParam *ip = reinterpret_cast<IntParam *>(p);
						datum.param_ref = ip;
						datum.stats = ip->access_counts();
						datum.value = ip->value();
					} break;

					case BOOL_PARAM: {
						BoolParam *ip = reinterpret_cast<BoolParam *>(p);
						datum.param_ref = ip;
						datum.stats = ip->access_counts();
						datum.value = ip->value();
					} break;

					case DOUBLE_PARAM: {
						DoubleParam *ip = reinterpret_cast<DoubleParam *>(p);
						datum.param_ref = ip;
						datum.stats = ip->access_counts();
						datum.value = ip->value();
					} break;

					case STRING_PARAM: {
						StringParam *ip = reinterpret_cast<StringParam *>(p);
						datum.param_ref = ip;
						datum.stats = ip->access_counts();
						datum.value = ip->value();
					} break;

					default: {
						datum.param_ref = p;
						datum.stats = p->access_counts();
						datum.value = p->value_str(Param::VALSTR_PURPOSE_RAW_DEFAULT_DATA_4_INSPECT);
					} break;
					}
				}
			}
		}

		if (globals) {
			for (ParamsVector *vec : globals->collection_) {
				for (auto i : vec->params_) {
					Snapshot::datums &datum = snap->data[item_idx++];
					ParamPtr p = i.second;
					switch (p->type()) {
					case INT_PARAM: {
						IntParam *ip = reinterpret_cast<IntParam *>(p);
						datum.param_ref = ip;
						datum.stats = ip->access_counts();
						datum.value = ip->value();
					} break;

					case BOOL_PARAM: {
						BoolParam *ip = reinterpret_cast<BoolParam *>(p);
						datum.param_ref = ip;
						datum.stats = ip->access_counts();
						datum.value = ip->value();
					} break;

					case DOUBLE_PARAM: {
						DoubleParam *ip = reinterpret_cast<DoubleParam *>(p);
						datum.param_ref = ip;
						datum.stats = ip->access_counts();
						datum.value = ip->value();
					} break;

					case STRING_PARAM: {
						StringParam *ip = reinterpret_cast<StringParam *>(p);
						datum.param_ref = ip;
						datum.stats = ip->access_counts();
						datum.value = ip->value();
					} break;

					default: {
						datum.param_ref = p;
						datum.stats = p->access_counts();
						datum.value = p->value_str(Param::VALSTR_PURPOSE_RAW_DATA_4_INSPECT);
					} break;
					}
				}
			}
		}

		return snap;
	}

	void Snapshot::ResetToSnapshot(Snapshot &snapshot) {
		for (Snapshot::datums &datum : snapshot.data) {
			ParamPtr p = datum.param_ref;

			// only reset the value (and count the write action!)
			// 
			// DO NOT reset the statistics!
			switch (p->type()) {
			case INT_PARAM: {
				IntParam *ip = reinterpret_cast<IntParam *>(p);
				ip->set_value(std::get<int32_t>(datum.value), PARAM_VALUE_IS_SET_BY_SNAPSHOT_REWIND);
			} break;

			case BOOL_PARAM: {
				BoolParam *ip = reinterpret_cast<BoolParam *>(p);
				ip->set_value(std::get<bool>(datum.value), PARAM_VALUE_IS_SET_BY_SNAPSHOT_REWIND);
			} break;

			case DOUBLE_PARAM: {
				DoubleParam *ip = reinterpret_cast<DoubleParam *>(p);
				ip->set_value(std::get<double>(datum.value), PARAM_VALUE_IS_SET_BY_SNAPSHOT_REWIND);
			} break;

			case STRING_PARAM: {
				StringParam *ip = reinterpret_cast<StringParam *>(p);
				ip->set_value(std::get<std::string>(datum.value), PARAM_VALUE_IS_SET_BY_SNAPSHOT_REWIND);
			} break;

			default: {
				datum.param_ref = p;
				p->set_value(std::get<std::string>(datum.value), PARAM_VALUE_IS_SET_BY_SNAPSHOT_REWIND);
			} break;
			}
		}
	}

	SnapshotSeries::~SnapshotSeries() {
		for (SnapshotPtr snap : series) {
			delete snap;
		}
		series.clear();
	}

	Snapshot &SnapshotSeries::TakeSnapshot(const char *name, ParamsVectorSet *globals, ParamsVectorSet *locals) {
		SnapshotPtr snap = Snapshot::TakeSnapshot(name, globals, locals);
		series.push_back(snap);
		return *snap;
	}

	void SnapshotSeries::RewindToSnapshot(Snapshot &snapshot) {
		SnapshotPtr snap = &snapshot;

		size_t i;
		for (i = series.size(); i > 0; i--) {
			SnapshotPtr si = series[i - 1];
			if (si == snap) {
				Snapshot::ResetToSnapshot(*si);
				break;
			}
			series[i - 1] = nullptr;
			delete si;
		}
		series.resize(i);
	}

	void SnapshotSeries::PopSnapshot() {
		size_t i = series.size();
		if (i > 0) {
			i--;
			SnapshotPtr si = series[i];
			series.pop_back();
			Snapshot::ResetToSnapshot(*si);
			delete si;
		}
	}

}	// namespace
