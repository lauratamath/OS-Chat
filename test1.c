#include <json-c/json.h>
#include <stdio.h>

void json_parse(json_object * jobj) {
 enum json_type type;
 json_object_object_foreach(jobj, key, val) {
 type = json_object_get_type(val);
 switch (type) {
 case json_type_string: printf("type: json_type_string, ");
 printf("value: %sn", json_object_get_string(val));
 break;
 }
 }
}

int main() {
 char * string = "{ "sitename" : "Joys of Programming", "purpose" : "programming tips","platform" : "wordpress"}";
 printf ("JSON string: %sn", string);
 json_object * jobj = json_tokener_parse(string);
 json_parse(jobj);
}
