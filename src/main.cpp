#if defined __APPLE__ || defined __unix__
#include <sys/ioctl.h>
#include <unistd.h>
#elif _WIN32
#include <winsock2.h>
#include <windows.h>
#endif
#define DATA_ERR_MSG "error: Failed to get data"
#define DEFAULT_COL 80
#define INDENT 4

#include <cpr/cpr.h>
#include <lexbor/dom/dom.h>
#include <lexbor/html/html.h>

#include <iomanip>
#include <iostream>
#include <string>

using std::cerr;
using std::cout, std::endl;
using std::size_t;
using std::string;

void check_status(const lxb_status_t& status, const string& err_msg)
{
    if (status != LXB_STATUS_OK) {
        cerr << err_msg << endl;
        exit(EXIT_FAILURE);
    }
}

lxb_dom_collection_t* get_collection(lxb_html_document_t* document)
{
    lxb_dom_collection_t* collection = lxb_dom_collection_make(&document->dom_document, 10);
    if (collection == nullptr) {
        cerr << "error: Failed to create collection data" << endl;
        exit(EXIT_FAILURE);
    }
    return collection;
}

// Get the first element
lxb_dom_element_t* get_element_by_class(lxb_html_document_t* document, lxb_dom_element_t* parent,
    const string& name)
{
    lxb_dom_collection_t* collection = get_collection(document);

    lxb_status_t status;
    status = lxb_dom_elements_by_class_name(parent, collection,
        reinterpret_cast<const lxb_char_t*>(name.c_str()), name.length());
    check_status(status, DATA_ERR_MSG);

    if (lxb_dom_collection_length(collection) == 0) {
        lxb_dom_collection_destroy(collection, true);
        return nullptr;
    }
    lxb_dom_element_t* data = lxb_dom_collection_element(collection, 0);
    lxb_dom_collection_destroy(collection, true);
    return data;
}

const lxb_char_t* get_text(const lxb_dom_element_t* element)
{
    return lxb_dom_node_text_content(lxb_dom_interface_node(element), nullptr);
}

inline string cast_string(const lxb_char_t* str)
{
    return string{reinterpret_cast<const char*>(str)};
}

unsigned short get_terminal_width()
{
#if defined __APPLE__ || defined __unix__
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
#elif _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left;   // Windows changes line, no +1 needed
#else
    return DEFAULT_COL;
#endif
}

// Indent from second line
void wrap_line(const string& text, const unsigned int width, const unsigned int indent_n)
{
    const auto indent = string(indent_n, ' ');

    std::istringstream stream{text};
    string word;
    unsigned int written_length{indent_n};

    bool line_beg = true;
    while (stream >> word) {
        if (word.length() + written_length + 1 > width) {
            cout << '\n' << indent;
            line_beg = true;
            written_length = indent_n;
        }
        if (line_beg) {
            cout << word;
            line_beg = false;
            written_length += word.length();
        } else {
            cout << ' ' << word;
            written_length += word.length() + 1;
        }
    }
    cout << endl;
}

string get_gram_grps(lxb_html_document_t *document, const lxb_dom_element_t* field)
{
    string grms {};
    lxb_dom_collection_t* grm_collection = get_collection(document);
    lxb_status_t status = lxb_dom_elements_by_class_name(lxb_dom_interface_element(field),
        grm_collection, reinterpret_cast<const lxb_char_t*>("pos"), 3);
    check_status(status, DATA_ERR_MSG);
    for (size_t i = 0; i < lxb_dom_collection_length(grm_collection); ++i)
    {
        if (i > 0) {
            grms += ", ";
        }
        grms += cast_string(get_text(lxb_dom_collection_element(grm_collection, i)));
    }
    return grms;
}

int main(const int argc, const char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    const auto w = get_terminal_width();
    const auto indent_str = string(INDENT, ' ');

    if (argc != 2) {
        cerr << "error: Incorrect number of arguments" << endl;
        cerr << "usage: dict <word/phrase>" << endl;
        return EXIT_FAILURE;
    }
    const string lookup{argv[1]};
    const cpr::Response r = cpr::Get(cpr::Url{"https://www.collinsdictionary.com/us/search/"},
        cpr::Parameters({{"dictCode", "english"}, {"q", lookup}}),
        cpr::Header{{"User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:10.0)"
            " Gecko/20100101 Firefox/10.0}"}});

    if (r.status_code != 200) {
        cerr << "error: Failed to get web content" << endl;
        return 0;
    }
    const string text = r.text;
    lxb_status_t status;

    auto* html = reinterpret_cast<const lxb_char_t*>(text.c_str());
    lxb_html_document_t* document = lxb_html_document_create();
    status = lxb_html_document_parse(document, html, text.length() - 1);
    check_status(status, "error: Failed to parse webpage");

    // Get Collins default dictionary
    lxb_dom_element_t* data = lxb_dom_interface_element(document->body);
    lxb_dom_collection_t* dict_collection = get_collection(document);
    status = lxb_dom_elements_by_class_name(lxb_dom_interface_element(data), dict_collection,
        reinterpret_cast<const lxb_char_t*>("cobuild"), 7);
    check_status(status, DATA_ERR_MSG);

    // The word does not exist in dictionary
    if (lxb_dom_collection_length(dict_collection) == 0) {
        cout << "No word or phrase named \"" << lookup << '"' << endl;

        data = get_element_by_class(document, data, "suggested_words");
        if (data != nullptr) {
            lxb_dom_collection_t* alt_collection = get_collection(document);
            status = lxb_dom_elements_by_tag_name(data, alt_collection,
                reinterpret_cast<const lxb_char_t*>("li"), 2);
            check_status(status, DATA_ERR_MSG);
            
            cout << "Maybe you mean:" << endl;
            // Print in two columns
            auto size = static_cast<unsigned int>(lxb_dom_collection_length(alt_collection));
            unsigned int half = size / 2;
            for (unsigned int i = 0; i < half; ++i) {
                cout << std::left << std::setw(w / 2);
                cout << get_text(lxb_dom_collection_element(alt_collection, i));
                cout << get_text(lxb_dom_collection_element(alt_collection, half + i));
                cout << endl;
            }
            if (size % 2 == 1) {
                cout << get_text(lxb_dom_collection_element(alt_collection, half)) << endl;
            }
            lxb_dom_collection_destroy(alt_collection, true);
        }
        lxb_html_document_destroy(document);
        return 0;
    }
    
    for (size_t i = 0; i < lxb_dom_collection_length(dict_collection); i += 2)
    {
        lxb_dom_element_t* dict = lxb_dom_collection_element(dict_collection, i);

        if (i > 0) {
            for (int i = 0; i < w; ++i) {
                cout << "―";   // horizontal bar
            }
            cout << endl;
        }

        // Get word
        lxb_dom_element_t* title_field = get_element_by_class(document, dict, "title_container");
        if (title_field == nullptr) {
            cout << "✦ " << get_text(get_element_by_class(document, dict, "h2_entry"));
        } else {
            data = get_element_by_class(document, title_field, "orth");
            cout << "✦ " << get_text(data);
        }

        // Get pronunciation
        data = get_element_by_class(document, dict, "mini_h2");
        lxb_dom_collection_t* pron_collection = get_collection(document);
        status = lxb_dom_elements_by_tag_name(lxb_dom_interface_element(data), pron_collection,
            reinterpret_cast<const lxb_char_t*>("span"), 4);
        check_status(status, DATA_ERR_MSG);
        if (lxb_dom_collection_length(pron_collection) == 0) {
            data = get_element_by_class(document, dict, "note");
            if (data != nullptr) {
                lxb_dom_collection_destroy(pron_collection, true);
                pron_collection = get_collection(document);
                status = lxb_dom_elements_by_tag_name(lxb_dom_interface_element(data), pron_collection,
                    reinterpret_cast<const lxb_char_t*>("span"), 4);
                check_status(status, DATA_ERR_MSG);
            }
        }

        for (size_t i = 0; i < lxb_dom_collection_length(pron_collection); ++i) {
            data = lxb_dom_collection_element(pron_collection, i);
            size_t len;
            string value = cast_string(lxb_dom_element_get_attribute(data,
                reinterpret_cast<const lxb_char_t*>("class"), 5, &len));
            if (value == "pron") {
                string pron = cast_string(get_text(data));
                pron = pron.substr(0, pron.length() - 3);
                size_t pos = pron.find_last_of(' ');
                if (pos != string::npos) {
                    pron = pron.substr(pos + 1);
                }
                cout << " [" << pron << ']';
            } else {
                if (value == "orth" && i > 0) {
                    break;
                }
            }
        }
        cout << endl;
        lxb_dom_collection_destroy(pron_collection, true);

        // Get generalization
        if (title_field != nullptr) {
            data = get_element_by_class(document, title_field, "lbl");
            if (data != nullptr) {
                string gen = cast_string(get_text(data));
                gen = gen.substr(1);
                cout << '\n' << gen << endl;
            }
        }

        // Get word forms
        data = get_element_by_class(document, dict, "type-infl");
        if (data != nullptr) {
            cout << "Word forms: ";
            lxb_dom_collection_t* form_collection = get_collection(document);
            status = lxb_dom_elements_by_class_name(lxb_dom_interface_element(data), form_collection,
                reinterpret_cast<const lxb_char_t*>("orth"), 4);
            check_status(status, DATA_ERR_MSG);
            cout << get_text(lxb_dom_collection_element(form_collection, 0));
            for (size_t i = 1; i < lxb_dom_collection_length(form_collection); ++i) {
                cout << ", " << get_text(lxb_dom_collection_element(form_collection, i));
            }
            cout << endl;
            lxb_dom_collection_destroy(form_collection, true);
        }

        // Get definitions
        lxb_dom_collection_t* def_collection = get_collection(document);
        status = lxb_dom_elements_by_class_name(lxb_dom_interface_element(dict), def_collection,
            reinterpret_cast<const lxb_char_t*>("hom"), 3);
        check_status(status, DATA_ERR_MSG);

        if (lxb_dom_collection_length(def_collection) == 0) {
            data = get_element_by_class(document, dict, "xr");
            cout << '\n';
            wrap_line(cast_string(get_text(data)), w, 0);
            break;
        }

        for (size_t i = 0; i < lxb_dom_collection_length(def_collection); ++i) {
            data = lxb_dom_collection_element(def_collection, i);

            lxb_dom_element_t* field;

            // Get definition
            string def;
            field = get_element_by_class(document, data, "def");
            if (field != nullptr) {
                def = cast_string(get_text(field));
            } else {
                field = get_element_by_class(document, data, "xr");
                if (field == nullptr) {
                    break;   // Empty definition
                }
                def = cast_string(get_text(field));
            }

            // Get grammar group
            cout << '\n';
            string def_num = std::to_string(i + 1) + ". ";
            if (lxb_dom_collection_length(def_collection) > 1) {
                cout << def_num;
            }
            field = get_element_by_class(document, data, "gramGrp");
            if (field != nullptr) {
                lxb_dom_element_t* grm = get_element_by_class(document, data, "pos");
                if (grm == nullptr) {
                    // Definition only
                    if (lxb_dom_collection_length(def_collection) > 1) {
                        wrap_line(def, w, def_num.length());
                    } else {
                        wrap_line(def, w, 0);
                    }
                } else if (grm == field) {
                    // No grammar group listing
                    cout << get_text(grm);
                    cout << '\n' << indent_str;
                    wrap_line(def, w, INDENT);
                } else {
                    cout << get_gram_grps(document, field);
                    cout << '\n' << indent_str;
                    wrap_line(def, w, INDENT);
                }
            } else {
                // Definition only
                if (lxb_dom_collection_length(def_collection) > 1) {
                    wrap_line(def, w, def_num.length());
                } else {
                    wrap_line(def, w, 0);
                }
            }

            // Get example
            field = get_element_by_class(document, data, "quote");
            if (field != nullptr) {
                string example = cast_string(get_text(field));
                cout << '\n' << indent_str << "-> ";
                wrap_line(example.substr(1), w, INDENT + 3);
            }

            // Get derivation
            lxb_dom_collection_t* drv_collection = get_collection(document);
            status = lxb_dom_elements_by_class_name(lxb_dom_interface_element(data), drv_collection,
                reinterpret_cast<const lxb_char_t*>("type-drv"), 8);
            check_status(status, DATA_ERR_MSG);
            if (lxb_dom_collection_length(drv_collection) != 0) {
                cout << '\n' << indent_str << "Derivations:" << endl;
            }
            for (size_t i = 0; i < lxb_dom_collection_length(drv_collection); ++i) {
                if (i % 2 == 1) {
                    continue;
                }
                field = lxb_dom_collection_element(drv_collection, i);
                cout << indent_str << "- " <<
                    get_text(get_element_by_class(document, field, "orth")) << " (" <<
                    get_gram_grps(document, field) << ')' << endl;
            }

            // Get synonym
            field = get_element_by_class(document, data, "thes");
            if (field != nullptr) {
                cout << indent_str << "Synonyms: ";

                lxb_dom_collection_t* syn_collection = get_collection(document);
                lxb_status_t status = lxb_dom_elements_by_class_name(field, syn_collection,
                    reinterpret_cast<const lxb_char_t*>("ref"), 3);
                check_status(status, DATA_ERR_MSG);
                cout << get_text(lxb_dom_collection_element(syn_collection, 0));
                for (size_t i = 1; i < lxb_dom_collection_length(syn_collection) - 1; ++i) {
                    cout << ", " << get_text(lxb_dom_collection_element(syn_collection, i));
                }
                cout << endl;
                lxb_dom_collection_destroy(syn_collection, true);
            }
            lxb_dom_collection_destroy(drv_collection, true);
        }
        lxb_dom_collection_destroy(def_collection, true);
    }

    lxb_html_document_destroy(document);
    return 0;
}
