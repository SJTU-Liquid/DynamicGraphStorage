#include "workload_generator.hpp"
#include "parser.hpp"

int main (int argc, char *argv[]) {
    Parser& parser = Parser::get_instance();
    parser.parse(argv[1]);

    workloadGenerator generator(parser);
    generator.generate_all(parser.get_output_dir());

    return 0;
}
