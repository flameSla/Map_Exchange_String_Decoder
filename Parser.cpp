#include "Parser.h"
#include <cassert>

#define assertm(exp, msg) assert(((void)msg, exp))

///////////////////////////////////////////////
Parser::Parser( std::string &MapString )
{
    Parser::MapString = MapString;
    pos = 0;
    last_position.x = 0;
    last_position.y = 0;

    string version = "version: " + to_string( read<uint16_t>() ) + "." + to_string( read<uint16_t>() ) + "." + to_string( read<uint16_t>() ) + "." + to_string( read<uint16_t>() );
    root["version"] = version;
    root["unknown"] = read<uint8_t>();
    read_map_gen_settings(); root["map_gen_settings"] = map_gen_settings;
    read_map_settings(); root["map_settings"] = map_settings;
    root["checksum"] =  read<uint32_t>();

    assertm( pos == MapString.length(), "data after end" );
}
///////////////////////////////////////////////
Parser::~Parser()
{
    //dtor
}
///////////////////////////////////////////////
template <class T> T Parser::read()
{
    bool ok = false;
    T result;

    size_t s = sizeof( T );
    size_t new_pos = pos + s;
    if(new_pos <= MapString.length())
    {
        result = *( ( T *) &MapString.c_str()[pos] );
        pos = new_pos;
        ok = true;
    }

    assertm( ok == true, "template <class T> T Parser::read()" );
    return result;
}
///////////////////////////////////////////////
template <class T> T Parser::read_optional()
{
    bool no_value = ( read<uint8_t>() == 0 );

    if ( no_value ) return 0;
    else return read<T>();
}
///////////////////////////////////////////////
string Parser::read_string()
{
    string result = "";
    uint32_t s = read_uint32so();
    size_t new_pos = pos + s;
    if(new_pos <= MapString.length())
    {
        for(uint32_t i=0;i < s; i++)
            result += MapString[pos+i];

        pos = new_pos;
    }
    else
    {
        assertm(1==0,"string Parser::read_string()");
    }
    return result;
}
///////////////////////////////////////////////
uint32_t Parser::read_uint32so()
{
    uint32_t result;
    uint8_t value = read<uint8_t>();
    if (value == 0xff)
    {
        result = read<uint32_t>();
    }
    else
    {
        result = value;
    }

    return result;
}
///////////////////////////////////////////////
bool Parser::read_bool()
{
    return read<uint8_t>() != 0;
}
///////////////////////////////////////////////
json Parser::read_map_position()
{
    int32_t x, y;
    int16_t x_diff, y_diff;

    x_diff = read<int16_t>() / 256;
    if ( x_diff == 0x7fff / 256)
    {
        x = read<int32_t>() / 256;
        y = read<int32_t>() / 256;
    }
    else
    {
        y_diff = read<int16_t>() / 256;
        x = last_position.x + x_diff;
        y = last_position.y + y_diff;
    }
    last_position.x = x;
    last_position.x = y;

    json result;
    result["x"] = x;
    result["y"] = y;

    return result;
}
///////////////////////////////////////////////
ostream& operator<<(ostream& os, const Parser &p)
{
    os << "root\r\n" << setw(4) << p.root << endl << endl;

    return os;
}
///////////////////////////////////////////////
void Parser::read_map_gen_settings()
{
    map_gen_settings["terrain_segmentation"] = read<float>();
    map_gen_settings["water"] = read<float>();

    string key;
    uint32_t imax = read_uint32so();
    json autoplace_controls;
    json autoplace_control;
    for(uint32_t i=0; i < imax; i++)
    {
        key = read_string();

        autoplace_control["frequency"]  = read<float>();
        autoplace_control["size"] = read<float>();
        autoplace_control["richness"] = read<float>();

        autoplace_controls[key] = autoplace_control;
    }
    if( !autoplace_controls.is_null() )
        map_gen_settings["autoplace_controls"] = autoplace_controls;

    imax = read_uint32so();
    json autoplace_settings, settings_value;
    for(uint32_t i=0; i < imax; i++)
    {
        settings_value.clear();
        key = read_string();
        settings_value["treat_missing_as_default"] = read_bool();
        {
            string key;
            uint32_t imax = read_uint32so();
            json setting_value;
            for(uint32_t i=0; i < imax; i++)
            {
                key = read_string();

                setting_value["frequency"] = read<float>();
                setting_value["size"] = read<float>();
                setting_value["richness"] = read<float>();

                settings_value["settings"][key] = setting_value;
            }
        }

        autoplace_settings[key] = settings_value;
    }
    if( !autoplace_settings.is_null() )
        map_gen_settings["autoplace_settings"] = autoplace_settings;

    map_gen_settings["default_enable_all_autoplace_controls"] = read_bool();
    map_gen_settings["seed"] = read<uint32_t>();
    map_gen_settings["width"] = read<uint32_t>();
    map_gen_settings["height"] = read<uint32_t>();

    map_gen_settings["area_to_generate_at_start"]["left_top"] = read_map_position();
    map_gen_settings["area_to_generate_at_start"]["right_bottom"] = read_map_position();
    map_gen_settings["area_to_generate_at_start"]["orientation"]["x"] = read<int16_t>();
    map_gen_settings["area_to_generate_at_start"]["orientation"]["y"] = read<int16_t>();
    map_gen_settings["starting_area"] = read<float>();
    map_gen_settings["peaceful_mode"] = read_bool();

    imax = read_uint32so();
    for(uint32_t i=0; i < imax; i++)
    {
        map_gen_settings["starting_points"].push_back( read_map_position() );
    }

    imax = read_uint32so();
    for(uint32_t i=0; i < imax; i++)
    {
        key = read_string();
        string string_value = read_string();

        map_gen_settings["property_expression_names"][key] = string_value;
    }

    map_gen_settings["cliff_settings"]["name"] = read_string();
    map_gen_settings["cliff_settings"]["cliff_elevation_0"]  = read<float>();
    map_gen_settings["cliff_settings"]["cliff_elevation_interval"]  = read<float>();
    map_gen_settings["cliff_settings"]["richness"]  = read<float>();
}
///////////////////////////////////////////////
void Parser::read_map_settings()
{
    map_settings["pollution"]["enabled"] = read_optional<bool>();
    map_settings["pollution"]["diffusion_ratio"] = read_optional<double>();
    map_settings["pollution"]["min_to_diffuse"] = read_optional<double>();
    map_settings["pollution"]["ageing"] = read_optional<double>();
    map_settings["pollution"]["expected_max_per_chunk"] = read_optional<double>();
    map_settings["pollution"]["min_to_show_per_chunk"] = read_optional<double>();
    map_settings["pollution"]["min_pollution_to_damage_trees"] = read_optional<double>();
    map_settings["pollution"]["pollution_with_max_forest_damage"] = read_optional<double>();
    map_settings["pollution"]["pollution_per_tree_damage"] = read_optional<double>();
    map_settings["pollution"]["pollution_restored_per_tree_damage"] = read_optional<double>();
    map_settings["pollution"]["max_pollution_to_restore_trees"] = read_optional<double>();
    map_settings["pollution"]["enemy_attack_pollution_consumption_modifier"] = read_optional<double>();

    map_settings["steering"]["default"]["radius"] = read_optional<double>();
    map_settings["steering"]["default"]["separation_factor"] = read_optional<double>();
    map_settings["steering"]["default"]["separation_force"] = read_optional<double>();
    map_settings["steering"]["default"]["force_unit_fuzzy_goto_behavior"] = read_optional<bool>();
    map_settings["steering"]["moving"]["radius"] = read_optional<double>();
    map_settings["steering"]["moving"]["separation_factor"] = read_optional<double>();
    map_settings["steering"]["moving"]["separation_force"] = read_optional<double>();
    map_settings["steering"]["moving"]["force_unit_fuzzy_goto_behavior"] = read_optional<bool>();

    map_settings["enemy_evolution"]["enabled"] = read_optional<bool>();
    map_settings["enemy_evolution"]["time_factor"] = read_optional<double>();
    map_settings["enemy_evolution"]["destroy_factor"] = read_optional<double>();
    map_settings["enemy_evolution"]["pollution_factor"] = read_optional<double>();

    map_settings["enemy_expansion"]["enabled"] = read_optional<bool>();
    map_settings["enemy_expansion"]["max_expansion_distance"] = read_optional<uint32_t>();
    map_settings["enemy_expansion"]["friendly_base_influence_radius"] = read_optional<uint32_t>();
    map_settings["enemy_expansion"]["enemy_building_influence_radius"] = read_optional<uint32_t>();
    map_settings["enemy_expansion"]["building_coefficient"] = read_optional<double>();
    map_settings["enemy_expansion"]["other_base_coefficient"] = read_optional<double>();
    map_settings["enemy_expansion"]["neighbouring_chunk_coefficient"] = read_optional<double>();
    map_settings["enemy_expansion"]["neighbouring_base_chunk_coefficient"] = read_optional<double>();
    map_settings["enemy_expansion"]["max_colliding_tiles_coefficient"] = read_optional<double>();
    map_settings["enemy_expansion"]["settler_group_min_size"] = read_optional<uint32_t>();
    map_settings["enemy_expansion"]["settler_group_max_size"] = read_optional<uint32_t>();
    map_settings["enemy_expansion"]["min_expansion_cooldown"] = read_optional<uint32_t>();
    map_settings["enemy_expansion"]["max_expansion_cooldown"] = read_optional<uint32_t>();

    map_settings["unit_group"]["min_group_gathering_time"] = read_optional<uint32_t>();
    map_settings["unit_group"]["max_group_gathering_time"] = read_optional<uint32_t>();
    map_settings["unit_group"]["max_wait_time_for_late_members"] = read_optional<uint32_t>();
    map_settings["unit_group"]["max_group_radius"] = read_optional<double>();
    map_settings["unit_group"]["min_group_radius"] = read_optional<double>();
    map_settings["unit_group"]["max_member_speedup_when_behind"] = read_optional<double>();
    map_settings["unit_group"]["max_member_slowdown_when_ahead"] = read_optional<double>();
    map_settings["unit_group"]["max_group_slowdown_factor"] = read_optional<double>();
    map_settings["unit_group"]["max_group_member_fallback_factor"] = read_optional<double>();
    map_settings["unit_group"]["member_disown_distance"] = read_optional<double>();
    map_settings["unit_group"]["tick_tolerance_when_member_arrives"] = read_optional<uint32_t>();
    map_settings["unit_group"]["max_gathering_unit_groups"] = read_optional<uint32_t>();
    map_settings["unit_group"]["max_unit_group_size"] = read_optional<uint32_t>();

    map_settings["path_finder"]["fwd2bwd_ratio"] = read_optional<int32_t>();
    map_settings["path_finder"]["goal_pressure_ratio"] = read_optional<double>();
    map_settings["path_finder"]["use_path_cache"] = read_optional<bool>();
    map_settings["path_finder"]["max_steps_worked_per_tick"] = read_optional<double>();
    map_settings["path_finder"]["max_work_done_per_tick"] = read_optional<uint32_t>();
    map_settings["path_finder"]["short_cache_size"] = read_optional<uint32_t>();
    map_settings["path_finder"]["long_cache_size"] = read_optional<uint32_t>();
    map_settings["path_finder"]["short_cache_min_cacheable_distance"] = read_optional<double>();
    map_settings["path_finder"]["short_cache_min_algo_steps_to_cache"] = read_optional<uint32_t>();
    map_settings["path_finder"]["long_cache_min_cacheable_distance"] = read_optional<double>();
    map_settings["path_finder"]["cache_max_connect_to_cache_steps_multiplier"] = read_optional<uint32_t>();
    map_settings["path_finder"]["cache_accept_path_start_distance_ratio"] = read_optional<double>();
    map_settings["path_finder"]["cache_accept_path_end_distance_ratio"] = read_optional<double>();
    map_settings["path_finder"]["negative_cache_accept_path_start_distance_ratio"] = read_optional<double>();
    map_settings["path_finder"]["negative_cache_accept_path_end_distance_ratio"] = read_optional<double>();
    map_settings["path_finder"]["cache_path_start_distance_rating_multiplier"] = read_optional<double>();
    map_settings["path_finder"]["cache_path_end_distance_rating_multiplier"] = read_optional<double>();
    map_settings["path_finder"]["stale_enemy_with_same_destination_collision_penalty"] = read_optional<double>();
    map_settings["path_finder"]["ignore_moving_enemy_collision_distance"] = read_optional<double>();
    map_settings["path_finder"]["enemy_with_different_destination_collision_penalty"] = read_optional<double>();
    map_settings["path_finder"]["general_entity_collision_penalty"] = read_optional<double>();
    map_settings["path_finder"]["general_entity_subsequent_collision_penalty"] = read_optional<double>();
    map_settings["path_finder"]["extended_collision_penalty"] = read_optional<double>();
    map_settings["path_finder"]["max_clients_to_accept_any_new_request"] = read_optional<uint32_t>();
    map_settings["path_finder"]["max_clients_to_accept_short_new_request"] = read_optional<uint32_t>();
    map_settings["path_finder"]["direct_distance_to_consider_short_request"] = read_optional<uint32_t>();
    map_settings["path_finder"]["short_request_max_steps"] = read_optional<uint32_t>();
    map_settings["path_finder"]["short_request_ratio"] = read_optional<double>();
    map_settings["path_finder"]["min_steps_to_check_path_find_termination"] = read_optional<uint32_t>();
    map_settings["path_finder"]["start_to_goal_cost_multiplier_to_terminate_path_find"] = read_optional<double>();
    if(read<uint8_t>() != 0)
    {
        uint32_t imax = read_uint32so();
        for (uint32_t i = 0; i < imax; i++)
        {
            map_settings["path_finder"]["overload_levels"].push_back( read<uint32_t>() );
        }
    }
    if(read<uint8_t>() != 0)
    {
        uint32_t imax = read_uint32so();
        for (uint32_t i = 0; i < imax; i++)
        {
            map_settings["path_finder"]["overload_multipliers"].push_back( read<double>() );
        }
    }
    map_settings["path_finder"]["negative_path_cache_delay_interval"] = read_optional<uint32_t>();

    map_settings["max_failed_behavior_count"] = read<uint32_t>();

    map_settings["difficulty_settings"]["recipe_difficulty"] = read<uint8_t>();
    map_settings["difficulty_settings"]["technology_difficulty"] = read<uint8_t>();
    map_settings["difficulty_settings"]["technology_price_multiplier"] = read<double>();
    string queue_setting[] = {"always", "after-victory", "never"};
    map_settings["difficulty_settings"]["research_queue_setting"] = queue_setting[ read<uint8_t>() ];
}
///////////////////////////////////////////////

/*
class Parser {
    constructor(buf) {
        this.pos = 0;
        this.buf = buf;
        this.last_position = { x: 0, y: 0 };
    }
}

function read_bool(parser) {
    let value = read_uint8(parser) !== 0;
    return value;
}

function read_uint8(parser) {
    let value = parser.buf.readUInt8(parser.pos);
    parser.pos += 1;
    return value;
}

function read_int16(parser) {
    let value = parser.buf.readInt16LE(parser.pos);
    parser.pos += 2;
    return value;
}

function read_uint16(parser) {
    let value = parser.buf.readUInt16LE(parser.pos);
    parser.pos += 2;
    return value;
}

function read_int32(parser) {
    let value = parser.buf.readInt32LE(parser.pos);
    parser.pos += 4;
    return value;
}

function read_uint32(parser) {
    let value = parser.buf.readUInt32LE(parser.pos);
    parser.pos += 4;
    return value;
}

function read_uint32so(parser) {
    let value = read_uint8(parser);
    if (value === 0xff) {
        return read_uint32(parser);
    }

    return value;
}

function read_float(parser) {
    let value = parser.buf.readFloatLE(parser.pos);
    parser.pos += 4;
    return value;
}

function read_double(parser) {
    let value = parser.buf.readDoubleLE(parser.pos);
    parser.pos += 8;
    return value;
}

function read_string(parser) {
    let size = read_uint32so(parser);
    let data = parser.buf.slice(parser.pos, parser.pos + size).toString("utf-8");
    parser.pos += size;
    return data;
}

function read_optional(parser, read_value) {
    let load = read_uint8(parser) !== 0;
    if (!load) {
        return null;
    }
    return read_value(parser);
}

function read_array(parser, read_item) {
    let size = read_uint32so(parser);

    let array = [];
    for (let i = 0; i < size; i++) {
        let item = read_item(parser);
        array.push(item);
    }

    return array;
}

function read_dict(parser, read_key, read_value) {
    let size = read_uint32so(parser);

    let mapping = new Map();
    for (let i = 0; i < size; i++) {
        let key = read_key(parser);
        let value = read_value(parser);
        mapping.set(key, value);
    }

    return mapping;
}

function read_version(parser) {
    let major = read_uint16(parser);
    let minor = read_uint16(parser);
    let patch = read_uint16(parser);
    let developer = read_uint16(parser);
    return [major, minor, patch, developer];
}

function read_frequency_size_richness(parser) {
    return {
        frequency: read_float(parser),
        size: read_float(parser),
        richness: read_float(parser),
    }
}

function read_autoplace_setting(parser) {
    return {
        treat_missing_as_default: read_bool(parser),
        settings: map_to_object(read_dict(parser, read_string, read_frequency_size_richness)),
    };
}

function read_map_position(parser) {
    let x, y;
    let x_diff = read_int16(parser) / 256;
    if (x_diff === 0x7fff / 256) {
        x = read_int32(parser) / 256;
        y = read_int32(parser) / 256;
    } else {
        let y_diff = read_int16(parser) / 256;
        x = parser.last_position.x + x_diff;
        y = parser.last_position.y + y_diff;
    }
    parser.last_position.x = x;
    parser.last_position.x = y;
    return { x, y };
}

function read_bounding_box(parser) {
    return {
        left_top: read_map_position(parser),
        right_bottom: read_map_position(parser),
        orientation: {
            x: read_int16(parser),
            y: read_int16(parser)
        },
    };
}

function read_cliff_settings(parser) {
    return {
        name: read_string(parser),
        elevation_0: read_float(parser),
        elevation_interval: read_float(parser),
        richness: read_float(parser),
    };
}

function map_to_object(map) {
    let obj = {};
    for (let [key, value] of map) {
        obj[key] = value;
    }
    return obj;
}

function read_map_gen_settings(parser) {
    return {
        terrain_segmentation: read_float(parser),
        water: read_float(parser),
        autoplace_controls: map_to_object(read_dict(parser, read_string, read_frequency_size_richness)),
        autoplace_settings: map_to_object(read_dict(parser, read_string, read_autoplace_setting)),
        default_enable_all_autoplace_controls: read_bool(parser),
        seed: read_uint32(parser),
        width: read_uint32(parser),
        height: read_uint32(parser),
        area_to_generate_at_start: read_bounding_box(parser),
        starting_area: read_float(parser),
        peaceful_mode: read_bool(parser),
        starting_points: read_array(parser, read_map_position),
        property_expression_names: map_to_object(read_dict(parser, read_string, read_string)),
        cliff_settings: read_cliff_settings(parser),
    };
}

function read_pollution(parser) {
    let enabled;

    return {
        enabled: read_optional(parser, read_bool),
        diffusion_ratio: read_optional(parser, read_double),
        min_to_diffuse: read_optional(parser, read_double),
        ageing: read_optional(parser, read_double),
        expected_max_per_chunk: read_optional(parser, read_double),
        min_to_show_per_chunk: read_optional(parser, read_double),
        min_pollution_to_damage_trees: read_optional(parser, read_double),
        pollution_with_max_forest_damage: read_optional(parser, read_double),
        pollution_per_tree_damage: read_optional(parser, read_double),
        pollution_restored_per_tree_damage: read_optional(parser, read_double),
        max_pollution_to_restore_trees: read_optional(parser, read_double),
        enemy_attack_pollution_consumption_modifier: read_optional(parser, read_double),
    };
}

function read_real_steering(parser) {
    return {
        radius: read_optional(parser, read_double),
        separation_factor: read_optional(parser, read_double),
        separation_force: read_optional(parser, read_double),
        force_unit_fuzzy_goto_behavior: read_optional(parser, read_bool),
    };

}

function read_steering(parser) {
    return {
        default: read_real_steering(parser),
        moving: read_real_steering(parser),
    };
}

function read_enemy_evolution(parser) {
    return {
        enabled: read_optional(parser, read_bool),
        time_factor: read_optional(parser, read_double),
        destroy_factor: read_optional(parser, read_double),
        pollution_factor: read_optional(parser, read_double),
    };
}

function read_enemy_expansion(parser) {
    return {
        enabled: read_optional(parser, read_bool),
        max_expansion_distance: read_optional(parser, read_uint32),
        friendly_base_influence_radius: read_optional(parser, read_uint32),
        enemy_building_influence_radius: read_optional(parser, read_uint32),
        building_coefficient: read_optional(parser, read_double),
        other_base_coefficient: read_optional(parser, read_double),
        neighbouring_chunk_coefficient: read_optional(parser, read_double),
        neighbouring_base_chunk_coefficient: read_optional(parser, read_double),
        max_colliding_tiles_coefficient: read_optional(parser, read_double),
        settler_group_min_size: read_optional(parser, read_uint32),
        settler_group_max_size: read_optional(parser, read_uint32),
        min_expansion_cooldown: read_optional(parser, read_uint32),
        max_expansion_cooldown: read_optional(parser, read_uint32),
    };
}

function read_unit_group(parser) {
    return {
        min_group_gathering_time: read_optional(parser, read_uint32),
        max_group_gathering_time: read_optional(parser, read_uint32),
        max_wait_time_for_late_members: read_optional(parser, read_uint32),
        max_group_radius: read_optional(parser, read_double),
        min_group_radius: read_optional(parser, read_double),
        max_member_speedup_when_behind: read_optional(parser, read_double),
        max_member_slowdown_when_ahead: read_optional(parser, read_double),
        max_group_slowdown_factor: read_optional(parser, read_double),
        max_group_member_fallback_factor: read_optional(parser, read_double),
        member_disown_distance: read_optional(parser, read_double),
        tick_tolerance_when_member_arrives: read_optional(parser, read_uint32),
        max_gathering_unit_groups: read_optional(parser, read_uint32),
        max_unit_group_size: read_optional(parser, read_uint32),
    };
}

function read_path_finder(parser) {
    return {
        fwd2bwd_ratio: read_optional(parser, read_int32),
        goal_pressure_ratio: read_optional(parser, read_double),
        use_path_cache: read_optional(parser, read_bool),
        max_steps_worked_per_tick: read_optional(parser, read_double),
        max_work_done_per_tick: read_optional(parser, read_uint32),
        short_cache_size: read_optional(parser, read_uint32),
        long_cache_size: read_optional(parser, read_uint32),
        short_cache_min_cacheable_distance: read_optional(parser, read_double),
        short_cache_min_algo_steps_to_cache: read_optional(parser, read_uint32),
        long_cache_min_cacheable_distance: read_optional(parser, read_double),
        cache_max_connect_to_cache_steps_multiplier: read_optional(parser, read_uint32),
        cache_accept_path_start_distance_ratio: read_optional(parser, read_double),
        cache_accept_path_end_distance_ratio: read_optional(parser, read_double),
        negative_cache_accept_path_start_distance_ratio: read_optional(parser, read_double),
        negative_cache_accept_path_end_distance_ratio: read_optional(parser, read_double),
        cache_path_start_distance_rating_multiplier: read_optional(parser, read_double),
        cache_path_end_distance_rating_multiplier: read_optional(parser, read_double),
        stale_enemy_with_same_destination_collision_penalty: read_optional(parser, read_double),
        ignore_moving_enemy_collision_distance: read_optional(parser, read_double),
        enemy_with_different_destination_collision_penalty: read_optional(parser, read_double),
        general_entity_collision_penalty: read_optional(parser, read_double),
        general_entity_subsequent_collision_penalty: read_optional(parser, read_double),
        extended_collision_penalty: read_optional(parser, read_double),
        max_clients_to_accept_any_new_request: read_optional(parser, read_uint32),
        max_clients_to_accept_short_new_request: read_optional(parser, read_uint32),
        direct_distance_to_consider_short_request: read_optional(parser, read_uint32),
        short_request_max_steps: read_optional(parser, read_uint32),
        short_request_ratio: read_optional(parser, read_double),
        min_steps_to_check_path_find_termination: read_optional(parser, read_uint32),
        start_to_goal_cost_multiplier_to_terminate_path_find: read_optional(parser, read_double),
        overload_levels: read_optional(parser, (p) => read_array(p, read_uint32)),
        overload_multipliers: read_optional(parser, (p) => read_array(p, read_double)),
        negative_path_cache_delay_interval: read_optional(parser, read_uint32),
    };
}

function read_difficulty_settings(parser) {
    return {
        recipe_difficulty: read_uint8(parser),
        technology_difficulty: read_uint8(parser),
        technology_price_multiplier: read_double(parser),
        research_queue_setting: ["always", "after-victory", "never"][read_uint8(parser)],
    };
}

function read_map_settings(parser) {
    return {
        pollution: read_pollution(parser),
        steering: read_steering(parser),
        enemy_evolution: read_enemy_evolution(parser),
        enemy_expansion: read_enemy_expansion(parser),
        unit_group: read_unit_group(parser),
        path_finder: read_path_finder(parser),
        max_failed_behavior_count: read_uint32(parser),
        difficulty_settings: read_difficulty_settings(parser),
    };
}

function decode(s) {
    s = s.replace(/[ \t\n\r]+/g, "");
    if (!/>>>[0-9a-zA-Z\/+]+={0,3}<<</.test(s)) {
        return "Not a map exchange string";
    }

    let buf = Buffer.from(s.slice(3, -3), "base64");
    buf = zlib.inflateSync(buf);

    let parser = new Parser(buf);

    let data = {
        version: read_version(parser),
        unknown: read_uint8(parser),
        map_gen_settings: read_map_gen_settings(parser),
        map_settings: read_map_settings(parser),
        checksum: read_uint32(parser),
    };

    if (parser.pos != buf.length) {
        return "data after end";
    }

    return data;
}
*/
