#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

class column;

class Param {
public:

	Param(std::string param) : _param(std::move(param)) {}
	Param(const char* param) : _param(param) {}
	Param(std::string_view param) : _param(param) {}

	auto operator()() const -> std::string {
		return param();
	}
	[[nodiscard]] auto param() const -> std::string {
		return _param;
	}

private:

	const std::string _param;
};

template<typename T>
inline auto to_value(const T& data) -> std::string {
	return std::to_string(data);
}

template<size_t N>
inline auto to_value(char const (&data)[N]) -> std::string {
	std::string str("'");
	str.append(data);
	str.append("'");
	return str;
}

template<>
inline auto to_value<std::string>(const std::string& data) -> std::string {
	std::string str("'");
	str.append(data);
	str.append("'");
	return str;
}

template<>
inline auto to_value<const char*>(const char* const& data) -> std::string {
	std::string str("'");
	str.append(data);
	str.append("'");
	return str;
}

template<>
inline auto to_value<Param>(const Param& data) -> std::string {
	return data();
}

template<>
inline auto to_value<column>(const column& data) -> std::string;

/*
template <>
static std::string to_value<time_t>(const time_t& data) {
	char buff[128] = {0};
	struct tm* ttime = localtime(&data);
	strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", ttime);
	std::string str("'");
	str.append(buff);
	str.append("'");
	return str;
}
*/

template<typename T>
void join_vector(std::string& result, const std::vector<T>& vec, const char* sep) {
	size_t size = vec.size();
	for(size_t i = 0; i < size; ++i) {
		if(i < size - 1) {
			result.append(vec[i]);
			result.append(sep);
		} else {
			result.append(vec[i]);
		}
	}
}

class column {
public:

	column(std::string_view column) {
		_cond = column;
	}
	virtual ~column() = default;

	auto as(std::string_view s) -> column& {
		_cond.append(" as ");
		_cond.append(s);
		return *this;
	}

	auto is_null() -> column& {
		_cond.append(" is null");
		return *this;
	}

	auto is_not_null() -> column& {
		_cond.append(" is not null");
		return *this;
	}

	template<typename T>
	auto in(const std::vector<T>& args) -> column& {
		size_t size = args.size();
		if(size == 1) {
			_cond.append(" = ");
			_cond.append(to_value(args[0]));
		} else {
			_cond.append(" in (");
			for(size_t i = 0; i < size; ++i) {
				if(i < size - 1) {
					_cond.append(to_value(args[i]));
					_cond.append(", ");
				} else {
					_cond.append(to_value(args[i]));
				}
			}
			_cond.append(")");
		}
		return *this;
	}

	template<typename T>
	auto not_in(const std::vector<T>& args) -> column& {
		size_t size = args.size();
		if(size == 1) {
			_cond.append(" != ");
			_cond.append(to_value(args[0]));
		} else {
			_cond.append(" not in (");
			for(size_t i = 0; i < size; ++i) {
				if(i < size - 1) {
					_cond.append(to_value(args[i]));
					_cond.append(", ");
				} else {
					_cond.append(to_value(args[i]));
				}
			}
			_cond.append(")");
		}
		return *this;
	}

	auto operator&&(column& condition) -> column& {
		std::string str("(");
		str.append(_cond);
		str.append(") and (");
		str.append(condition._cond);
		str.append(")");
		condition._cond = str;
		return condition;
	}

	auto operator||(column& condition) -> column& {
		std::string str("(");
		str.append(_cond);
		str.append(") or (");
		str.append(condition._cond);
		str.append(")");
		condition._cond = str;
		return condition;
	}

	auto operator&&(std::string_view condition) -> column& {
		_cond.append(" and ");
		_cond.append(condition);
		return *this;
	}

	auto operator||(std::string_view condition) -> column& {
		_cond.append(" or ");
		_cond.append(condition);
		return *this;
	}

	auto operator&&(const char* condition) -> column& {
		_cond.append(" and ");
		_cond.append(condition);
		return *this;
	}

	auto operator||(const char* condition) -> column& {
		_cond.append(" or ");
		_cond.append(condition);
		return *this;
	}

	template<typename T>
	auto operator==(const T& data) -> column& {
		_cond.append(" = ");
		_cond.append(to_value(data));
		return *this;
	}

	template<typename T>
	auto operator!=(const T& data) -> column& {
		_cond.append(" != ");
		_cond.append(to_value(data));
		return *this;
	}

	template<typename T>
	auto operator>=(const T& data) -> column& {
		_cond.append(" >= ");
		_cond.append(to_value(data));
		return *this;
	}

	template<typename T>
	auto operator<=(const T& data) -> column& {
		_cond.append(" <= ");
		_cond.append(to_value(data));
		return *this;
	}

	template<typename T>
	auto operator>(const T& data) -> column& {
		_cond.append(" > ");
		_cond.append(to_value(data));
		return *this;
	}

	template<typename T>
	auto operator<(const T& data) -> column& {
		_cond.append(" < ");
		_cond.append(to_value(data));
		return *this;
	}

	[[nodiscard]] auto str() const -> std::string_view {
		return _cond;
	}

	operator bool() {
		return true;
	}

private:

	std::string _cond;
};

template<>
inline auto to_value<column>(const column& data) -> std::string {
	return std::string(data.str());
}

class SqlModel {
public:

	SqlModel()			= default;
	virtual ~SqlModel() = default;

	virtual auto str() -> std::string_view = 0;
	auto last_sql() -> std::string_view {
		return _sql;
	}

	SqlModel(const SqlModel& m)						  = delete;
	auto operator=(const SqlModel& data) -> SqlModel& = delete;

protected:

	std::string _sql;
};

class SelectModel : public SqlModel {
public:

	SelectModel()			= default;
	~SelectModel() override = default;

	template<typename... Args>
	auto select(std::string_view str, Args&&... columns) -> SelectModel& {
		_select_columns.emplace_back(str);
		select(columns...);
		return *this;
	}

	// for recursion
	auto select() -> SelectModel& {
		return *this;
	}

	auto distinct() -> SelectModel& {
		_distinct = true;
		return *this;
	}

	template<typename... Args>
	auto from(std::string_view table_name, Args&&... tables) -> SelectModel& {
		if(_table_name.empty()) {
			_table_name = table_name;
		} else {
			_table_name.append(", ");
			_table_name.append(table_name);
		}
		from(tables...);
		return *this;
	}

	// for recursion
	auto from() -> SelectModel& {
		return *this;
	}

	auto join(std::string_view table_name) -> SelectModel& {
		_join_type	= "join";
		_join_table = table_name;
		return *this;
	}

	auto left_join(std::string_view table_name) -> SelectModel& {
		_join_type	= "left join";
		_join_table = table_name;
		return *this;
	}

	auto left_outer_join(std::string_view table_name) -> SelectModel& {
		_join_type	= "left outer join";
		_join_table = table_name;
		return *this;
	}

	auto right_join(std::string_view table_name) -> SelectModel& {
		_join_type	= "right join";
		_join_table = table_name;
		return *this;
	}

	auto right_outer_join(std::string_view table_name) -> SelectModel& {
		_join_type	= "right outer join";
		_join_table = table_name;
		return *this;
	}

	auto full_join(std::string_view table_name) -> SelectModel& {
		_join_type	= "full join";
		_join_table = table_name;
		return *this;
	}

	auto full_outer_join(std::string_view table_name) -> SelectModel& {
		_join_type	= "full outer join";
		_join_table = table_name;
		return *this;
	}

	auto on(std::string_view condition) -> SelectModel& {
		_join_on_condition.emplace_back(condition);
		return *this;
	}

	auto on(const column& condition) -> SelectModel& {
		_join_on_condition.emplace_back(condition.str());
		return *this;
	}

	auto where(std::string_view condition) -> SelectModel& {
		_where_condition.emplace_back(condition);
		return *this;
	}

	auto where(const column& condition) -> SelectModel& {
		_where_condition.emplace_back(condition.str());
		return *this;
	}

	template<typename... Args>
	auto group_by(std::string_view str, Args&&... columns) -> SelectModel& {
		_groupby_columns.emplace_back(str);
		group_by(columns...);
		return *this;
	}

	// for recursion
	auto group_by() -> SelectModel& {
		return *this;
	}

	auto having(std::string_view condition) -> SelectModel& {
		_having_condition.emplace_back(condition);
		return *this;
	}

	auto having(const column& condition) -> SelectModel& {
		_having_condition.emplace_back(condition.str());
		return *this;
	}

	auto order_by(std::string_view order_by) -> SelectModel& {
		_order_by = order_by;
		return *this;
	}

	template<typename T>
	auto limit(const T& limit) -> SelectModel& {
		_limit = std::to_string(limit);
		return *this;
	}
	template<typename T>
	auto limit(const T& offset, const T& limit) -> SelectModel& {
		_offset = std::to_string(offset);
		_limit	= std::to_string(limit);
		return *this;
	}
	template<typename T>
	auto offset(const T& offset) -> SelectModel& {
		_offset = std::to_string(offset);
		return *this;
	}

	auto str() -> std::string_view override {
		_sql.clear();
		_sql.append("select ");
		if(_distinct) {
			_sql.append("distinct ");
		}
		join_vector(_sql, _select_columns, ", ");
		_sql.append(" from ");
		_sql.append(_table_name);
		if(!_join_type.empty()) {
			_sql.append(" ");
			_sql.append(_join_type);
			_sql.append(" ");
			_sql.append(_join_table);
		}
		if(!_join_on_condition.empty()) {
			_sql.append(" on ");
			join_vector(_sql, _join_on_condition, " and ");
		}
		if(!_where_condition.empty()) {
			_sql.append(" where ");
			join_vector(_sql, _where_condition, " and ");
		}
		if(!_groupby_columns.empty()) {
			_sql.append(" group by ");
			join_vector(_sql, _groupby_columns, ", ");
		}
		if(!_having_condition.empty()) {
			_sql.append(" having ");
			join_vector(_sql, _having_condition, " and ");
		}
		if(!_order_by.empty()) {
			_sql.append(" order by ");
			_sql.append(_order_by);
		}
		if(!_limit.empty()) {
			_sql.append(" limit ");
			_sql.append(_limit);
		}
		if(!_offset.empty()) {
			_sql.append(" offset ");
			_sql.append(_offset);
		}
		return _sql;
	}

	auto reset() -> SelectModel& {
		_select_columns.clear();
		_distinct = false;
		_groupby_columns.clear();
		_table_name.clear();
		_join_type.clear();
		_join_table.clear();
		_join_on_condition.clear();
		_where_condition.clear();
		_having_condition.clear();
		_order_by.clear();
		_limit.clear();
		_offset.clear();
		return *this;
	}
	friend auto operator<<(std::ostream& out, SelectModel& mod) -> std::ostream& {
		out << mod.str();
		return out;
	}

protected:

	std::vector<std::string> _select_columns;
	bool _distinct{};
	std::vector<std::string> _groupby_columns;
	std::string _table_name;
	std::string _join_type;
	std::string _join_table;
	std::vector<std::string> _join_on_condition;
	std::vector<std::string> _where_condition;
	std::vector<std::string> _having_condition;
	std::string _order_by;
	std::string _limit;
	std::string _offset;
};

class InsertModel : public SqlModel {
public:

	InsertModel()			= default;
	~InsertModel() override = default;

	template<typename T>
	auto insert(std::string_view c, const T& data) -> InsertModel& {
		_columns.emplace_back(c);
		_values.push_back(to_value(data));
		return *this;
	}

	template<typename T>
	auto operator()(std::string_view c, const T& data) -> InsertModel& {
		return insert(c, data);
	}

	auto into(std::string_view table_name) -> InsertModel& {
		_table_name = table_name;
		return *this;
	}

	auto replace(bool var) -> InsertModel& {
		_replace = var;
		return *this;
	}

	auto str() -> std::string_view override {
		_sql.clear();
		std::string v_ss;

		if(_replace) {
			_sql.append("insert or replace into ");
		} else {
			_sql.append("insert into ");
		}

		_sql.append(_table_name);
		_sql.append("(");
		v_ss.append(" values(");
		size_t size = _columns.size();
		for(size_t i = 0; i < size; ++i) {
			if(i < size - 1) {
				_sql.append(_columns[i]);
				_sql.append(", ");
				v_ss.append(_values[i]);
				v_ss.append(", ");
			} else {
				_sql.append(_columns[i]);
				_sql.append(")");
				v_ss.append(_values[i]);
				v_ss.append(")");
			}
		}
		_sql.append(v_ss);
		return _sql;
	}

	auto reset() -> InsertModel& {
		_table_name.clear();
		_columns.clear();
		_values.clear();
		return *this;
	}

	friend auto operator<<(std::ostream& out, InsertModel& mod) -> std::ostream& {
		out << mod.str();
		return out;
	}

protected:

	bool _replace = false;
	std::string _table_name;
	std::vector<std::string> _columns;
	std::vector<std::string> _values;
};

template<>
inline auto InsertModel::insert(std::string_view c, const std::nullptr_t&) -> InsertModel& {
	_columns.emplace_back(c);
	_values.emplace_back("null");
	return *this;
}

class UpdateModel : public SqlModel {
public:

	UpdateModel()			= default;
	~UpdateModel() override = default;

	auto update(std::string_view table_name) -> UpdateModel& {
		_table_name = table_name;
		return *this;
	}

	template<typename T>
	auto set(std::string_view c, const T& data) -> UpdateModel& {
		std::string str(c);
		str.append(" = ");
		str.append(to_value(data));
		_set_columns.push_back(str);
		return *this;
	}

	template<typename T>
	auto operator()(std::string_view c, const T& data) -> UpdateModel& {
		return set(c, data);
	}

	auto where(std::string_view condition) -> UpdateModel& {
		_where_condition.emplace_back(condition);
		return *this;
	}

	auto where(const column& condition) -> UpdateModel& {
		_where_condition.emplace_back(condition.str());
		return *this;
	}

	auto str() -> std::string_view override {
		_sql.clear();
		_sql.append("update ");
		_sql.append(_table_name);
		_sql.append(" set ");
		join_vector(_sql, _set_columns, ", ");
		size_t size = _where_condition.size();
		if(size > 0) {
			_sql.append(" where ");
			join_vector(_sql, _where_condition, " and ");
		}
		return _sql;
	}

	auto reset() -> UpdateModel& {
		_table_name.clear();
		_set_columns.clear();
		_where_condition.clear();
		return *this;
	}
	friend auto operator<<(std::ostream& out, UpdateModel& mod) -> std::ostream& {
		out << mod.str();
		return out;
	}

protected:

	std::vector<std::string> _set_columns;
	std::string _table_name;
	std::vector<std::string> _where_condition;
};

template<>
inline auto UpdateModel::set(std::string_view c, const std::nullptr_t&) -> UpdateModel& {
	std::string str(c);
	str.append(" = null");
	_set_columns.push_back(str);
	return *this;
}

class DeleteModel : public SqlModel {
public:

	DeleteModel()			= default;
	~DeleteModel() override = default;

	auto _delete() -> DeleteModel& {
		return *this;
	}

	template<typename... Args>
	auto from(std::string_view table_name, Args&&... tables) -> DeleteModel& {
		if(_table_name.empty()) {
			_table_name = table_name;
		} else {
			_table_name.append(", ");
			_table_name.append(table_name);
		}
		from(tables...);
		return *this;
	}

	// for recursion
	auto from() -> DeleteModel& {
		return *this;
	}

	auto where(std::string_view condition) -> DeleteModel& {
		_where_condition.emplace_back(condition);
		return *this;
	}

	auto where(const column& condition) -> DeleteModel& {
		_where_condition.emplace_back(condition.str());
		return *this;
	}

	auto str() -> std::string_view override {
		_sql.clear();
		_sql.append("delete from ");
		_sql.append(_table_name);
		size_t size = _where_condition.size();
		if(size > 0) {
			_sql.append(" where ");
			join_vector(_sql, _where_condition, " and ");
		}
		return _sql;
	}

	auto reset() -> DeleteModel& {
		_table_name.clear();
		_where_condition.clear();
		return *this;
	}
	friend auto operator<<(std::ostream& out, DeleteModel& mod) -> std::ostream& {
		out << mod.str();
		return out;
	}

protected:

	std::string _table_name;
	std::vector<std::string> _where_condition;
};
