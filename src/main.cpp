#include <cpr/cpr.h>
#include <lexbor/html/html.h>
#include <lexbor/dom/dom.h>

#include <iostream>

enum access { CLASS, TAG };

void check_status(const lxb_status_t &status, const std::string &err_msg)
{
    if (status != LXB_STATUS_OK) {
        std::cerr << err_msg << std::endl;
        exit(EXIT_FAILURE);
    }
}

lxb_dom_collection_t * get_collection(lxb_html_document_t *document)
{
    lxb_dom_collection_t *collection = lxb_dom_collection_make(&document->dom_document, 10);
    if (collection == nullptr) {
        std::cerr << "Failed to create collection data!" << std::endl;
        exit(EXIT_FAILURE);
    }
    return collection;
}

// Get the first element
lxb_dom_element_t * get_element(const access &option, lxb_html_document_t *document, lxb_dom_element_t *parent
    , const std::string &name)
{
    lxb_dom_collection_t *collection = get_collection(document);

    lxb_status_t status;
    switch (option) {
        case CLASS:
            status = lxb_dom_elements_by_class_name(parent, collection
                , reinterpret_cast<const lxb_char_t *>(name.c_str()), name.size());
            break;
        case TAG:
            status = lxb_dom_elements_by_tag_name(parent, collection
                , reinterpret_cast<const lxb_char_t *>(name.c_str()), name.size());
            break;
        default:
            std::cerr << "Unknown access option!" << std::endl;
            exit(EXIT_FAILURE);
    }
    check_status(status, "Failed to get data!");

    if (lxb_dom_collection_length(collection) == 0) {
        lxb_dom_collection_destroy(collection, true);
        return nullptr;
    }
    lxb_dom_element_t *data = lxb_dom_collection_element(collection, 0);
    lxb_dom_collection_destroy(collection, true);
    return data;
}

lxb_dom_element_t * get_element_by_class(lxb_html_document_t *document, lxb_dom_element_t *parent
    , const std::string &name)
{
    return get_element(access{CLASS}, document, parent, name);
}

lxb_dom_element_t * get_element_by_tag(lxb_html_document_t *document, lxb_dom_element_t *parent
    , const std::string &name)
{
    return get_element(access{TAG}, document, parent, name);
}

const lxb_char_t * get_text(const lxb_dom_element_t *element)
{
    return lxb_dom_node_text_content(lxb_dom_interface_node(element), nullptr);
}

std::string cast_string(const lxb_char_t *str)
{
    return std::string{reinterpret_cast<const char *>(str)};
}

std::string & format(std::string &str)
{
    unsigned long i = 0;
    while (i < str.length()) {
        if (!std::isspace(str[i])) {
            break;
        }
        ++i;
    }
    unsigned long j = str.length();
    while (j > 0) {
        if (!std::isspace(str[j])) {
            break;
        }
        --j;
    }
    str = str.substr(i, j);
    std::string res;
    i = 0;
    while (i < str.length()) {
        if (std::isspace(str[i])) {
            res += ' ';
            do
                ++i;
            while (std::isspace(str[i]));
        } else {
            res += str[i];
            ++i;
        }
    }
    str = res;
    return str;
}

void get_synonym(lxb_html_document_t *document, lxb_dom_element_t *data)
{
    lxb_dom_element_t *field = get_element_by_class(document, data, "thes");
    if (field != nullptr) {
        std::cout << "    Synonyms: ";

        lxb_dom_collection_t *collection = get_collection(document);
        lxb_status_t status = lxb_dom_elements_by_class_name(field, collection
            , reinterpret_cast<const lxb_char_t *>("ref"), 3);
        check_status(status, "Failed to get data");
        std::cout << get_text(lxb_dom_collection_element(collection, 0));
        for (unsigned int j = 1; j < lxb_dom_collection_length(collection) - 1; ++j) {
            std::cout << ", " << get_text(lxb_dom_collection_element(collection, j));
        }
        std::cout << std::endl;
        lxb_dom_collection_destroy(collection, true);
    }
}

void print_string(const std::string &text)
{
    const unsigned max_line_length(75);
    const std::string line_prefix("    ");

    std::istringstream text_iss(text);

    std::string word;
    unsigned characters_written = 0;

    std::cout << line_prefix;
    while (text_iss >> word) {
        if (word.size() + characters_written > max_line_length) {
            std::cout << '\n' << line_prefix;
            characters_written = 0;
        }

        std::cout << word << ' ';
        characters_written += word.size() + 1;
    }
    std::cout << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Incorrect number of arguments" << std::endl;
        std::cerr << "Usage: dict <word/phrase>" << std::endl;
        return EXIT_FAILURE;
    }
    std::string lookup{argv[1]};
    const cpr::Response r = cpr::Get(cpr::Url{"https://www.collinsdictionary.com/us/search/"},
                                     cpr::Parameters({{"dictCode", "english"}, {"q", lookup}}),
                                     cpr::Header{{"User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:10.0)"
                                                                " Gecko/20100101 Firefox/10.0}"}});

    if (r.status_code != 200) {
        std::cerr << "Could not connect to website" << std::endl;
        return 0;
    }
    std::string text = r.text;
    lxb_status_t status;

    auto *html = reinterpret_cast<const lxb_char_t *>(text.c_str());
    lxb_html_document_t *document = lxb_html_document_create();
    status = lxb_html_document_parse(document, html, text.length() - 1);
    check_status(status, "Failed to parse webpage");

    // Get Collins default dictionary
    lxb_dom_element_t *data = lxb_dom_interface_element(document->body);
    lxb_dom_element_t *dict = get_element_by_class(document, data, "cobuild");

    // The word does not exist in dictionary
    if (dict == nullptr) {
        dict = get_element_by_class(document, data, "american");
        if (dict == nullptr) {
            std::cout << "No word or phrase named \"" << lookup << '"' << std::endl;

            data = get_element_by_class(document, data, "suggested_words");
            if (data != nullptr) {
                lxb_dom_collection_t *collection = get_collection(document);
                status = lxb_dom_elements_by_tag_name(data, collection
                    , reinterpret_cast<const lxb_char_t *>("li"), 2);
                check_status(status, "Failed to get data");
                std::cout << "Maybe you mean:" << std::endl;
                // Print in two columns
                unsigned int size = lxb_dom_collection_length(collection);
                unsigned int half = size / 2;
                for (unsigned int i = 0; i < half; ++i) {
                    std::cout << std::left << std::setw(45)
                              << get_text(lxb_dom_collection_element(collection, i));
                    std::cout << get_text(lxb_dom_collection_element(collection, half + i));
                    std::cout << std::endl;
                }
                if (size % 2 == 1) {
                    std::cout << get_text(lxb_dom_collection_element(collection, half)) << std::endl;
                }
                lxb_dom_collection_destroy(collection, true);
            }
            lxb_html_document_destroy(document);
            return 0;
        }
    }

    // Get word
    data = get_element_by_tag(document, dict, "h2");
    std::cout << get_text(data);

    // Get pronunciation
    data = get_element_by_class(document, dict, "pron");
    if (data != nullptr) {
        std::string pron{reinterpret_cast<const char *>(get_text(data))};
        pron = pron.substr(0, pron.length() - 3);
        std::cout << " [" << pron << ']';
    }
    std::cout << std::endl;

    // Get word forms
    data = get_element_by_class(document, dict, "type-infl");
    if (data != nullptr) {
        std::cout << get_text(data) << std::endl;
    }

    // Get types
    lxb_dom_collection_t *collection = get_collection(document);
    status = lxb_dom_elements_by_class_name(lxb_dom_interface_element(dict), collection
        , reinterpret_cast<const lxb_char_t *>("hom"), 3);
    check_status(status, "Failed to get data");
    for (unsigned int i = 0; i < lxb_dom_collection_length(collection); ++i) {
        data = lxb_dom_collection_element(collection, i);

        lxb_dom_element_t *field;

        // Get numbering
        field = get_element_by_class(document, data, "sensenum");
        if (field != nullptr) {
            std::string num = reinterpret_cast<const char *>(get_text(field));
            std::cout << '\n' << num.substr(0, num.length() - 2);
        }

        // Get grammar group
        field = get_element_by_class(document, data, "gramGrp");
        if (field != nullptr) {
            if (lxb_dom_collection_length(collection) != 1) {
                std::cout << ' ';
            }
            std::string gram_grp = cast_string(get_text(field));
            if (gram_grp[0] == ' ') {
                std::cout << gram_grp.substr(1, gram_grp.length()) << std::endl;
            } else {
                std::cout << gram_grp << std::endl;
            }
        }

        // Get definition
        field = get_element_by_class(document, data, "def");
        if (field != nullptr) {
            std::string def = cast_string((get_text(field)));
            print_string(def);
        } else {
            field = get_element_by_class(document, data, "xr");
            std::string def = cast_string((get_text(field)));
            std::cout << ' ' << format(def) << std::endl;
        }

        // Get example
        field = get_element_by_class(document, data, "quote");
        if (field != nullptr) {
            std::string example = cast_string((get_text(field)));
            std::cout << '\n';
            print_string("-> " + example.substr(1));
        }

        // Get synonym
        get_synonym(document, data);

        // Get derivation
        lxb_dom_collection_t *drv_collection = get_collection(document);
        status = lxb_dom_elements_by_class_name(lxb_dom_interface_element(data), drv_collection
            , reinterpret_cast<const lxb_char_t *>("type-drv"), 8);
        check_status(status, "Failed to get data");
        if (lxb_dom_collection_length(drv_collection) != 0) {
            std::cout << "\n    Derivations:" << std::endl;
        }
        for (unsigned int j = 0; j < lxb_dom_collection_length(drv_collection); ++j) {
            if (j % 2 == 1) {
                continue;
            }
            field = lxb_dom_collection_element(drv_collection, j);
            std::string grp = cast_string(get_text(get_element_by_class(document, field, "gramGrp")));
            std::cout << "    - " << get_text(get_element_by_class(document, field, "orth"))
                      << " (" << grp.substr(1, grp.length()) << ")" << std::endl;
            std::string example = cast_string(get_text(get_element_by_class(document, field, "quote")));
            std::cout << '\n';
            print_string("-> " + example.substr(1));

            // Get synonym
            get_synonym(document, field);
        }
        lxb_dom_collection_destroy(drv_collection, true);
    }
    lxb_dom_collection_destroy(collection, true);
    lxb_html_document_destroy(document);
    return 0;
}
