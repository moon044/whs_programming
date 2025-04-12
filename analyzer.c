#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

// declname 찾기
const char* find_declname(json_t *type_node) {
    if (!type_node || !json_is_object(type_node)) return NULL;

    json_t *declname = json_object_get(type_node, "declname");
    if (declname && json_is_string(declname))
        return json_string_value(declname);

    json_t *inner = json_object_get(type_node, "type");
    return find_declname(inner);
}

// 리턴 타입 / 파라미터 타입 찾기
const char* find_type_name(json_t *type_node) {
    if (!type_node || !json_is_object(type_node)) return NULL;

    json_t *names = json_object_get(type_node, "names");
    if (names && json_is_array(names) && json_array_size(names) > 0)
        return json_string_value(json_array_get(names, 0));

    json_t *inner = json_object_get(type_node, "type");
    return find_type_name(inner);
}

// if 개수 세기
int count_if_recursive(json_t *node) {
    if (!node) return 0;
    int count = 0;

    if (json_is_object(node)) {
        json_t *nt = json_object_get(node, "_nodetype");
        if (nt && json_is_string(nt)) {
            if (strcmp(json_string_value(nt), "If") == 0)
                count++;
        }

        const char *key;
        json_t *value;
        json_object_foreach(node, key, value) {
            count += count_if_recursive(value);
        }
    } else if (json_is_array(node)) {
        for (size_t i = 0; i < json_array_size(node); i++) {
            count += count_if_recursive(json_array_get(node, i));
        }
    }

    return count;
}

// 함수 분석
void analyze_function(json_t *func_node, int index) {
    printf("★ 함수 %d\n", index);

    // 함수 선언
    json_t *decl = json_object_get(func_node, "decl");
    json_t *type_node = decl ? json_object_get(decl, "type") : NULL;

    // 이름
    const char *fname = find_declname(type_node);
    printf(" - 이름: %s\n", fname ? fname : "알 수 없음");

    // 리턴 타입
    const char *rtype = find_type_name(json_object_get(type_node, "type"));
    printf(" - 리턴 타입: %s\n", rtype ? rtype : "알 수 없음");

    // 파라미터
    json_t *args = json_object_get(type_node, "args");
    json_t *params = args ? json_object_get(args, "params") : NULL;

    if (params && json_is_array(params) && json_array_size(params) > 0) {
        printf(" - 파라미터:\n");
        for (size_t i = 0; i < json_array_size(params); i++) {
            json_t *param = json_array_get(params, i);
            const char *pname = json_string_value(json_object_get(param, "name"));
            const char *ptype = find_type_name(json_object_get(param, "type"));
            printf("   · %s %s\n", ptype ? ptype : "unknown", pname ? pname : "unnamed");
        }
    } else {
        printf(" - 파라미터: 없음\n");
    }

    // 조건문 개수
    json_t *body = json_object_get(func_node, "body");
    int ifs = count_if_recursive(body);
    printf(" - 조건문 개수 (if): %d\n\n", ifs);
}

int main() {
    json_error_t error;
    json_t *root = json_load_file("ast.json", 0, &error);
    if (!root) {
        fprintf(stderr, "X ast.json 로딩 실패: %s\n", error.text);
        return 1;
    }

    json_t *ext = json_object_get(root, "ext");
    if (!ext || !json_is_array(ext)) {
        fprintf(stderr, "X ext 항목 없음 또는 배열이 아님\n");
        json_decref(root);
        return 1;
    }

    int total_funcs = 0;
    int total_ifs = 0;

    for (size_t i = 0; i < json_array_size(ext); i++) {
        json_t *node = json_array_get(ext, i);
        const char *nt = json_string_value(json_object_get(node, "_nodetype"));
        if (nt && strcmp(nt, "FuncDef") == 0) {
            total_funcs++;
            total_ifs += count_if_recursive(json_object_get(node, "body"));
        }
    }

    printf("※ 총 함수 개수: %d\n", total_funcs);
    printf("※ 전체 if 조건문 개수: %d\n\n", total_ifs);

    int idx = 1;
    for (size_t i = 0; i < json_array_size(ext); i++) {
        json_t *node = json_array_get(ext, i);
        const char *nt = json_string_value(json_object_get(node, "_nodetype"));
        if (nt && strcmp(nt, "FuncDef") == 0) {
            analyze_function(node, idx++);
        }
    }

    json_decref(root);
    return 0;
}
