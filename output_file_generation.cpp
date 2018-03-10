#include "output_file_generation.h"
#include <iostream>

using namespace std;


static QString get_name_func_from_name_step ( QString name_step, QVector< vertex_struct >* tree ) {
	for ( int l = 0; l < tree->size(); l++ ) {
		if ( tree->at( l ).step_name == name_step ) {
			return tree->at( l ).func_name;
		}
	}
	return "";
}

static void abort_parsing ( QString& message, uint32_t code ) {
	QByteArray b_byte_string = message.toUtf8();
	char* b_write = b_byte_string.data();
	cout << b_write << endl;
	exit( code );
}

static void write_to_file ( QFile& f, QString& s ) {
	QByteArray b_byte_string = s.toUtf8();
	char* b_write = b_byte_string.data();
	f.write( b_write );
}

int create_output_file ( char* file_path, QString obj_class, QVector< vertex_struct >* tree, char* handler_class_name ) {
	QFile f( file_path );
	bool b_r;

	b_r = f.open( QIODevice::WriteOnly | QIODevice::Text );
	if ( !b_r ) return ENOENT;

	f.write( "#include \"fsm.h\"\n" );

	QString s_handler = "#include \"" + QString( handler_class_name ) + "\"\n\n";
	write_to_file( f, s_handler );

	/// Перечень шагов.
	for ( int l = 0; l < tree->size(); l++ ) {
		QString	extern_func_fsm;
		extern_func_fsm = "extern const fsm_step< ";
		extern_func_fsm += obj_class;
		extern_func_fsm += " > ";
		extern_func_fsm += obj_class;
		extern_func_fsm += "_";
		extern_func_fsm += tree->at( l ).func_name;
		extern_func_fsm += "_fsm_step;\n";

		if ( l == tree->size() - 1 ) {
			extern_func_fsm += "\n";
		}

		write_to_file( f, extern_func_fsm );
	}

	/// Начинаем описывать сами шаги.
	for ( int l = 0; l < tree->size(); l++ ) {
		// Если вершина хоть с кем-то связана.
		int vertex_count = tree->at( l ).vertex_connect->size();
		QString	extern_func_fsm_array;

		uint32_t count_intm_array = 0;			/// Реальное количество элементов в массиве. Включая nullptr.
		if ( vertex_count != 0 ) {
			/// Сортируем вектор переходов, чтобы потом найти в нем пробелы.
			std::sort( tree->at( l ).vertex_connect->begin(), tree->at( l ).vertex_connect->end() );
			/// Мы берем не колличество связок, а самый старший номер связки. Потому что может быть одна связка с номером
			/// 12, а 12 (0..11) до нее мы заполним nullptr.
			/// ( vertex_count - 1 ) - потому что у нас колличество элементов (счет с 1). А в векторе считаются с 0.
			/// .number + 1, потому что в переменной хранится номер перехода в логике вектора. Типа 0-й, 1-й.
			/// А нам надо колличество (счет с 1).
			count_intm_array = tree->at( l ).vertex_connect->at( vertex_count - 1 ).number + 1;

			extern_func_fsm_array = "const fsm_step< ";
			extern_func_fsm_array += obj_class;
			extern_func_fsm_array += " >* ";
			extern_func_fsm_array += obj_class;
			extern_func_fsm_array += "_";
			extern_func_fsm_array += tree->at( l ).func_name;
			extern_func_fsm_array += "_fsm_step_array[ ";
			extern_func_fsm_array += QString::number( count_intm_array );
			extern_func_fsm_array += " ] = {\n";

			/// Описываем все соединения.

			/// Защита на случай, что будет пробел в переходах.
			/// Пример, функция возвращает 0, 2, 3. А 1 - нет. Там должно быть nullptr.
			long int counting_control = -1;

			for ( int i = 0; i < vertex_count; ) {
				counting_control++;
				/// Пробел в векторе переходов.
				if ( counting_control !=  tree->at( l ).vertex_connect->at( i ).number ) {
					extern_func_fsm_array += "\tnullptr,\n";
					continue;
				}

				extern_func_fsm_array += "\t&";
				extern_func_fsm_array += obj_class;
				extern_func_fsm_array += "_";
				/// Ищем соответствующему имени шага имя функции.
				QString nm = get_name_func_from_name_step( tree->at( l ).vertex_connect->at( i ).connect_step, tree );

				/// Если вдруг нет такого шага.
				if ( nm.isEmpty() ) {
					QString b_s = "Name func for step <<" + tree->at( l ).vertex_connect->at( i ).connect_step + ">> does not exist!";
					abort_parsing( b_s, -1 );
				}

				extern_func_fsm_array += nm;
				extern_func_fsm_array += "_fsm_step";

				if ( i != vertex_count - 1 ) {
					extern_func_fsm_array += ",\n";
				} else {
					extern_func_fsm_array += "\n";
				}
				i++;
			}

			extern_func_fsm_array += "};\n\n";

			write_to_file( f, extern_func_fsm_array );
		}

		/// Сам элемент графа.
		QString	b_s_vertex;
		b_s_vertex = "const fsm_step< ";
		b_s_vertex += obj_class;
		b_s_vertex += " > ";
		b_s_vertex += obj_class;
		b_s_vertex += "_";
		b_s_vertex += tree->at( l ).func_name;
		b_s_vertex += "_fsm_step = {\n";

		b_s_vertex += "\t.func_step\t\t\t\t= ";
		b_s_vertex += obj_class;
		b_s_vertex += "::fsm_step_func_";
		b_s_vertex += tree->at( l ).func_name;
		b_s_vertex += ",\n";
		b_s_vertex += "\t.next_step_array\t\t= ";

		if ( vertex_count != 0 ) {
			b_s_vertex += obj_class;
			b_s_vertex += "_";
			b_s_vertex += tree->at( l ).func_name;
			b_s_vertex += "_fsm_step_array";
		} else {
			b_s_vertex += "nullptr";
		}

		b_s_vertex += ",\n";
		b_s_vertex += "\t.number_array\t\t\t= ";
		b_s_vertex += QString::number( count_intm_array );
		b_s_vertex += "\n};\n\n";

		write_to_file( f, b_s_vertex );
	}

	f.close();
	return 0;
}
