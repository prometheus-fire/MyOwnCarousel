#pragma once
#include <iostream>
#include <fstream>
#include <sstream>

#include <tao/pegtl.hpp>
#include <tao/pegtl/nothing.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

namespace openscad_loader {


    namespace pegtl = tao::pegtl;

    struct cr : pegtl::one< '\r' >
    {
    };

    // Rule to match a line feed (\n)
    struct lf : pegtl::one< '\n' >
    {
    };

    struct ws : pegtl::star< pegtl::sor<pegtl::space, cr, lf > >
    {
    };


    // Define grammar rules
    struct identifier : pegtl::plus< pegtl::alpha >
    {
    };  // Matches variable names

    struct integer : pegtl::plus< pegtl::digit >
    {
    };

    struct sign : pegtl::opt< pegtl::one< '-' >  >
    {
    };

    struct number : pegtl::seq <  sign, integer,
        pegtl::opt  <    pegtl::seq< pegtl::one<'.'>,
        integer >
        >
    >
    {
    };


    struct vec3 : pegtl::seq<   pegtl::string <'[' >,
        number,
        pegtl::one< ',' >,
        number,
        pegtl::one< ',' >,
        number,
        pegtl::string< ']' >
    >
    {
    };

    struct equals : pegtl::seq< pegtl::opt< pegtl::space >, pegtl::one< '=' >, pegtl::opt< pegtl::space > >
    {
    };  // Handles optional spaces

    struct semicolon : pegtl::one< ';' >
    {
    };
    struct assignment : pegtl::seq< identifier, equals, number, semicolon >
    {
    };  // Matches "var = 10;"


    struct name_cylinder : pegtl::string< 'c', 'y', 'l', 'i', 'n', 'd', 'e', 'r' >
    {
    };

    struct cylinder : pegtl::seq< name_cylinder, pegtl::one< '('>,
        number,
        pegtl::string< ')', ';' >
    >
    {
    };

    struct name_cube : pegtl::string< 'c', 'u', 'b', 'e' >
    {
    };

    struct cube : pegtl::seq< name_cube, pegtl::one< '('>, number, pegtl::string< ')', ';' >
    >
    {
    };

    struct name_sphere : pegtl::string< 's', 'p', 'h', 'e', 'r', 'e'>
    {
    };

    struct sphere : pegtl::seq< name_sphere, pegtl::one< '('>, number, pegtl::string< ')', ';' >
    >
    {
    };

    struct color : pegtl::seq<  pegtl::string< 'c', 'o', 'l', 'o', 'r'>, pegtl::string <'(', '['>,
        number,
        pegtl::one< ',' >,
        number,
        pegtl::one< ',' >,
        number,
        pegtl::one< ',' >,
        number,
        pegtl::string< ']', ')' >>

    {

    };

    struct rotate : pegtl::seq< pegtl::string< 'r', 'o', 't', 'a', 't', 'e'>, pegtl::string <'('>,
        vec3,
        pegtl::string<')' > >
    {
    };

    struct translate : pegtl::seq< pegtl::string< 't', 'r', 'a', 'n', 's', 'l', 'a', 't', 'e'>,
        pegtl::string <'('>,
        vec3,
        pegtl::string<')' > >
    {
    };
    struct node;


    struct transformation : pegtl::sor< rotate, translate >
    {
    };

    struct transformation_node : pegtl::seq<transformation, pegtl::one< '{' >, node, pegtl::one< '}' > >
    {
    };

    struct attribute : pegtl::sor< color >
    {
    };

    struct attribute_node : pegtl::seq<attribute, pegtl::one< '{' >, node, pegtl::one< '}' > >
    {
    };



    struct op_difference_name : pegtl::string< 'd', 'i', 'f', 'f', 'e', 'r', 'e', 'n', 'c', 'e' > {};
    struct op_intersection_name : pegtl::string< 'i', 'n', 't', 'e', 'r', 's', 'e', 'c', 't', 'i', 'o', 'n' > {};
    struct op_union_name : pegtl::string< 'u', 'n', 'i', 'o', 'n' > {};

    struct op_difference :pegtl::seq<op_difference_name, pegtl::string< '(', ')'>, pegtl::string<'{' >, node, node, pegtl::star<node>, pegtl::string< '}'>  > {};
    struct op_intersection :pegtl::seq<op_intersection_name, pegtl::string< '(', ')'>, pegtl::string<'{' >, node, node, pegtl::star<node>, pegtl::string< '}'>  > {};
    struct op_union : pegtl::seq<op_union_name, pegtl::string< '(', ')'>, pegtl::string<'{' >, node, node, pegtl::star<node>, pegtl::string< '}'>  > {};

    struct operation_node : pegtl::sor<op_union, op_difference, op_intersection> {};

    struct primitive : pegtl::sor< cylinder, cube, sphere >
    {
    };

    struct leaf_node : primitive
    {
    };


    struct inner_node :
        pegtl::sor<    transformation_node, attribute_node, operation_node >
    {
    };
    struct node : pegtl::sor< inner_node, leaf_node >
    {
    };


    struct grammar : pegtl::must< pegtl::seq<   node > >
    {
    };



    int cnt = 0;

    template< typename Rule >
    struct action
    {

    };

    // Specialisation of the user-defined action to do
    // something when the 'name' rule succeeds; is called
    // with the portion of the input that matched the rule.

    template<>
    struct action< inner_node >
    {
        template< typename ParseInput >
        static void apply(const ParseInput& in, std::string& v)
        {
            //        v += std::string("\n")+std::to_string(cnt++) + in.string();
            std::cout << in.string() << "\n";
        }
    };

    template<>
    struct action< cube >
    {
        template< typename ParseInput >
        static void apply(const ParseInput& in, std::string& v)
        {
            //       v += std::string("\n") + std::to_string(cnt++) + in.string();
            std::cout << in.string() << "\n";

        }
    };

    template<>
    struct action< cylinder >
    {
        template< typename ParseInput >
        static void apply(const ParseInput& in, std::string& v)
        {
            //        v += std::string("\n") + std::to_string(cnt++) + in.string();
            std::cout << in.string() << "\n";

        }
    };

    template<>
    struct action< sphere >
    {
        template< typename ParseInput >
        static void apply(const ParseInput& in, std::string& v)
        {
            //       v += std::string("\n") + std::to_string(cnt++) + in.string();
            std::cout << in.string() << "\n";

        }
    };


    template< typename Rule >
    using my_selector = tao::pegtl::parse_tree::selector< Rule,
        tao::pegtl::parse_tree::store_content::on<
        sphere,
        cube,
        cylinder,
        op_difference,
        op_intersection,
        op_union,
        attribute_node,
        transformation_node,
        operation_node,
        translate,
        rotate,
        number> >;


    struct op {
        op() { type = -1; }
        op(int t) :type(t) {}
        int type; //0 = union, 1 = intersection, 2 = difference
        
    };


    struct prim {
        prim() :m(glm::mat4(1.f)) {}
        glm::mat4 m;
        int type; // 0= sphere, 1=cube, 2=cylinder
        float data;
        int id_node;
        int _; //pad
    };

    struct prim_sphere : public prim {
        prim_sphere(float r) { data = r; }
    };

    struct prim_cube : public prim {
        prim_cube(float size) { data = size; }
    };

    struct prim_cylinder : public prim {
        prim_cylinder(float height) { data = height; }
    };


    class  loader {

        void visit_tree(std::unique_ptr<pegtl::parse_tree::node> node, glm::mat4 m, int pos_par) {

            std::cout << (node)->type << std::endl;

            if (node->is_type<cube>()) {
                float size = std::atof(node->children[0]->string().c_str());
                prim_cube* p = new prim_cube(size);
                p->m = m;
                p->type = 1;
                p->id_node =  pos_par   ;
                primitives.push_back( p );
                prim_cube* tmp = (prim_cube*)primitives.back();
            }
            else
                if (node->is_type<sphere>()) {
                    float size = std::atof(node->children[0]->string().c_str());
                    prim_sphere* p = new prim_sphere(size);
                    p->type = 0;
                    p->m = m;
                    p->id_node = pos_par ;
                    primitives.push_back(p);
                    prim_sphere* tmp = (prim_sphere*)primitives.back();
                }
                else
                    if (node->is_type<cylinder>()) {
                        float height = std::atof(node->children[0]->string().c_str());
                        float radius = std::atof(node->children[1]->string().c_str());
                        prim_cylinder* p = new prim_cylinder(height);
                        p->m = m;
                        p->type = 2;
                        p->id_node = pos_par ;
                        primitives.push_back( p);
                    }
                    else
                        if (node->is_type<transformation_node>()) {
                            std::unique_ptr<pegtl::parse_tree::node>  node_t = std::move(node->children[0]);
                            if (node_t->is_type<translate>()) {
                                glm::vec3 t;
                                t.x = std::atof(node_t->children[0]->string().c_str());
                                t.y = std::atof(node_t->children[1]->string().c_str());
                                t.z = std::atof(node_t->children[2]->string().c_str());
                                m = glm::translate(m, t);
                            }
                            else
                                if (node_t->is_type<rotate>()) {
                                    glm::vec3 r;
                                    r.x = std::atof(node_t->children[0]->string().c_str());
                                    r.y = std::atof(node_t->children[1]->string().c_str());
                                    r.z = std::atof(node_t->children[2]->string().c_str());
                                    m = glm::rotate(m, glm::radians(r.x), glm::vec3(1, 0, 0));
                                    m = glm::rotate(m, glm::radians(r.y), glm::vec3(0, 1, 0));
                                    m = glm::rotate(m, glm::radians(r.z), glm::vec3(0, 0, 1));
                                }
                            visit_tree(std::move((node->children[1])), m, pos_par);
                        }
                        else
                            if (node->is_type<operation_node>()) {
                                std::unique_ptr<pegtl::parse_tree::node>  node_t = std::move(node->children[0]);
                                if (node_t->is_type<op_union>()) {
                                    std::cout << "union" << std::endl;
//                                    operations.push_back(std::make_pair(pos_par, op(0)));
                                    operations[pos_par] = 0;
                                }
                                else
                                    if (node_t->is_type<op_intersection>()) {
                                        std::cout << "intersection" << std::endl;
//                                        operations.push_back(std::make_pair(pos_par, op(1)));
                                        operations[pos_par] = 1;
                                    }
                                    else
                                        if (node_t->is_type<op_difference>()) {
                                            std::cout << "difference" << std::endl;
//                                          operations.push_back(std::make_pair(pos_par, op(2)));
                                            operations[pos_par] = 2;
                                        }
                                visit_tree(std::move((node_t->children[0])), m, pos_par *2);
                                visit_tree(std::move((node_t->children[1])), m, pos_par * 2+1);
                            }
                            else
                                if (!node->children.empty()) {
                                    for (auto c = node->children.begin(); c != node->children.end();++c) {
                                        visit_tree(std::move(*c), m, pos_par);
                                    }
                                }
        }

        // Main function
    public:
        std::vector<  prim*  > primitives;
        std::vector< int >  operations;

        int load( const char* path)
        {
            std::string input;

            std::string filename(path);

            // Create an input file stream
            std::ifstream file(filename);

            // Check if the file was opened successfully
            if (!file) {
                std::cerr << "Could not open the file: " << filename << std::endl;
                return 1;
            }

            // Use a stringstream to read the entire file content into a string
            std::stringstream buffer;
            buffer << file.rdbuf();  // Read file content into the stringstream

            // Convert stringstream to a string
            input = buffer.str();
            input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
            input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
            input.erase(std::remove(input.begin(), input.end(), '\t'), input.end());
            input.erase(std::remove(input.begin(), input.end(), '\b'), input.end());
            input.erase(std::remove(input.begin(), input.end(), ' '), input.end());

            // Create memory input
            pegtl::memory_input<> in(input, "input");
            std::string name;
            try {
                // Parse with debugging output enabled
            //    pegtl::parse< grammar,action>( in,name);  // Using pegtl::nothing for actions
                auto root = tao::pegtl::parse_tree::parse< grammar, my_selector >(in);
                std::cout << "Parse successful!" << std::endl;
                std::cout << name;

                operations.resize(65536,-1);
                visit_tree(std::move(root), glm::mat4(1.0), 1);


                std::cout << "primitives" << std::endl;
                for (int i = 0; i < primitives.size(); ++i)
                    std::cout << i << " " << primitives[i]->type <<" "<< primitives[i]->id_node << std::endl;

                std::cout << "operations" << std::endl;
                for (int i = 0; i < 100; ++i)
                    std::cout << i << " " << operations[i] << std::endl;
            }
            catch (const pegtl::parse_error& e) {
                std::cerr << "Parse error: " << e.what() << std::endl;
            }

            return 0;
        }

    };




 


};
