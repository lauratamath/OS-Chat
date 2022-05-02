#include <json-c/json.h>
#include <stdio.h>

void json_parse_input( json_object *jobj )
{
    int exists, i, j, k, l;
    char *results;
    json_object *queriesObj, *resultsObj, *valuesObj, *tmpQueries, *tmpResults, *tmpValues, *tmpSeparateVals;

    /* Get query key */
    exists = json_object_object_get_ex( jobj, "queries", &queriesObj );
    if ( FALSE == exists )
    {
        printf( "\"queries\" not found in JSON\n" );
        return;
    }

    /* Loop through array of queries */
    for ( i = 0; i < json_object_array_length( queriesObj ); i++ )
    {
        tmpQueries = json_object_array_get_idx( queriesObj, i );

        /* Get results info */
        exists = json_object_object_get_ex( tmpQueries, "results", &resultsObj );
        if ( FALSE == exists )
        {
            printf( "\"results\" not found in JSON\n" );
            return;
        }

        /* Loop through array of results */
        for ( j = 0; j < json_object_array_length( resultsObj ); j++ )
        {
            tmpResults = json_object_array_get_idx ( resultsObj, j );

            /* Get values */
            exists = json_object_object_get_ex( tmpResults, "values", &valuesObj );
            if ( FALSE == exists )
            {
                printf( "\"values\" not found in JSON\n" );
                return;
            }

            /* Loop through array of array of values */
            for ( k = 0; k < json_object_array_length( valuesObj ); k++ )
            {
                tmpValues = json_object_array_get_idx ( valuesObj, k );

                /* Loop through array of values */
                for ( l = 0; l < json_object_array_length( tmpValues ); l++ )
                {
                    tmpSeparateVals = json_object_array_get_idx ( tmpValues, l );
                    printf( "Values:[%d] = %s \n", l, json_object_to_json_string( tmpSeparateVals ) );
                }
            }
        }
    }
}

int main()
{
    json_object *jobj;

    char * string = " { \"queries\" : [ { \"sample_size\" : 1, \"results\" : [ { \"name\" : \"data\", \"group_by\" : [{ \"name\" : \"type\", \"type\" : \"number\" }], \"tags\" : { \"hostname\" : [ \"host\" ]}, \"values\": [[1438775895302, 143]] } ], } ] } ";
    printf ( "JSON string: %s\n\n", string );

    jobj = json_tokener_parse( string );
    json_parse_input( jobj );
}