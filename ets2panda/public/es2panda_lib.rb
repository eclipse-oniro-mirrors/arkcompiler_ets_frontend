# Copyright (c) 2024-2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

require 'set'
require 'delegate'
require_relative 'enums.rb'

module Es2pandaLibApi
  class Arg
    # Containers
    @es2panda_arg = nil
    @lib_args = nil
    @idl_args = nil
    @lib_cast = nil
    @return_args = nil
    @idl_return_args = nil
    @is_ast_node = false
    @is_ast_node_add_children = false
    @is_ast_type = false
    @is_ast_type_add_children = false
    @is_var_type = false
    @is_enum_type = false
    @is_scope_type = false
    @is_code_gen = false
    @is_decl_type = false
    @is_optional = false
    @default_optional = false
    @const = ''
    @base_namespace = ''

    # Flags
    @need_var_cast = false

    def self.check_ptr_depth(change_type, ptr_depth)
      !((change_type.es2panda_arg['min_ptr_depth'] && ptr_depth < change_type.es2panda_arg['min_ptr_depth']) ||
      (change_type.es2panda_arg['max_ptr_depth'] && ptr_depth > change_type.es2panda_arg['max_ptr_depth']))
    end

    def delete_refs(args)
      args&.map do |arg|
        arg['type']['ref_depth'] = 0 if arg['type'].respond_to?('ref_depth')
      end
    end

    def delete_ref(type)
      type['ref_depth'] = 0 if type.respond_to?('ref_depth')
    end

    def self.const(es2panda_arg)
      return 'const' if es2panda_arg['type'].respond_to?('prefix') &&
                        es2panda_arg['type']['prefix'].include?('const') ||
                        es2panda_arg['type'].respond_to?('other_modifiers') &&
                        es2panda_arg['type']['other_modifiers'].include?('const') ||
                        es2panda_arg['type'].respond_to?('const') &&
                        es2panda_arg['type']['const']&.include?('const')
    end

    def self.set_const_modifier(args, value)
      args&.map do |arg|
        unless arg['type']&.respond_to?('const') && arg['type']&.respond_to?('ptr_depth') &&
               arg['type']&.ptr_depth&.positive?
          arg['type']['const'] = value || ''
        end
      end
    end

    def stop_modify_idl_arg
      @is_ast_node || @is_ast_node_add_children || @is_ast_type || @is_ast_type_add_children || @is_var_type ||
      @is_scope_type || @is_decl_type
    end

    def modify_template_nested_arg(arg, base_namespace, idl_mode = false)
      arg['type'] = ClassData.add_base_namespace(arg['type'], base_namespace)
      tmp = Arg.new(arg, base_namespace)
      raise "Unsupported double+ nested complex types: #{arg}\n" if tmp.lib_args.length > 1

      return nil if tmp.lib_args.nil? || tmp.lib_args[0].nil?
      return arg if idl_mode && tmp.stop_modify_idl_arg

      tmp.lib_args[0]['increase_ptr_depth'] = arg['increase_ptr_depth'] if (arg['increase_ptr_depth'] || 0) != 0
      tmp.lib_args[0]['type']['const'] = tmp.const
      tmp.lib_args[0]
    end

    def self.unsupported_type_msg(es2panda_arg)
      ptr_depth = es2panda_arg['type'].ptr_depth || 0
      namespace = es2panda_arg['type'].namespace
      "'#{namespace ? "#{namespace}::" : ''}"\
      "#{es2panda_arg['type'].name}"\
      "#{' ' * [1, ptr_depth].min + '*' * ptr_depth}'"
    end

    def self.is_ast_node(es2panda_arg)
      Es2pandaLibApi.ast_nodes.include?(es2panda_arg['type'].name) &&
        (!es2panda_arg['type']['namespace'] || es2panda_arg['type']['namespace'] == 'ir')
    end

    def self.is_ast_node_add_children(es2panda_arg)
      Es2pandaLibApi.ast_node_additional_children.include?(es2panda_arg['type'].name) &&
        (!es2panda_arg['type']['namespace'] || es2panda_arg['type']['namespace'] == 'ir')
    end

    def self.is_ast_type(es2panda_arg)
      Es2pandaLibApi.ast_types.include?(es2panda_arg['type'].name) &&
        (!es2panda_arg['type']['namespace'] || es2panda_arg['type']['namespace'] == 'checker')
    end

    def self.is_ast_type_add_children(es2panda_arg)
      Es2pandaLibApi.ast_type_additional_children.include?(es2panda_arg['type'].name) &&
        (!es2panda_arg['type']['namespace'] ||
        es2panda_arg['type']['namespace'] == 'checker')
    end

    def self.is_var_type(es2panda_arg)
      Es2pandaLibApi.ast_variables.any? { |variable| variable[1] == es2panda_arg['type'].name } &&
        (!es2panda_arg['type']['namespace'] || es2panda_arg['type']['namespace'] == 'varbinder')
    end

    def self.is_enum_type(es2panda_arg)
      Es2pandaLibApi.enums.include?(es2panda_arg['type'].name)
    end

    def self.is_scope_type(es2panda_arg)
      Es2pandaLibApi.scopes.include?(es2panda_arg['type'].name) &&
        (!es2panda_arg['type']['namespace'] || es2panda_arg['type']['namespace'] == 'varbinder')
    end

    def self.is_decl_type(es2panda_arg)
      Es2pandaLibApi.declarations.include?(es2panda_arg['type'].name) &&
        (!es2panda_arg['type']['namespace'] || es2panda_arg['type']['namespace'] == 'varbinder')
    end

    def self.is_code_gen(es2panda_arg)
      Es2pandaLibApi.code_gen_children.include?(es2panda_arg['type'].name) &&
        (!es2panda_arg['type']['namespace'] || es2panda_arg['type']['namespace'] == 'compiler')
    end

    def self.get_change_type_info(es2panda_arg)
      found_change_type_link = Es2pandaLibApi.change_types.find do |x|
        x.es2panda_arg.type.respond_to?('name') && x.es2panda_arg.type.name == es2panda_arg['type']['name'] &&
          (!x.es2panda_arg.type.respond_to?('namespace') ||
            x.es2panda_arg.type.namespace == es2panda_arg['type'].namespace) &&
          (!x.es2panda_arg.type.respond_to?('current_class') ||
            x.es2panda_arg.type['current_class'] == es2panda_arg['type']['current_class']) &&
          Arg.check_ptr_depth(x, es2panda_arg['type']['ptr_depth'] || 0)
      end

      return found_change_type_link if found_change_type_link

      if Arg.is_ast_type(es2panda_arg)
        found_change_type_link = Es2pandaLibApi.change_types.find do |x|
          x.es2panda_arg.type == '|AstType|' && Arg.check_ptr_depth(x, es2panda_arg['type'].ptr_depth || 0)
        end
      end

      if Arg.is_ast_type_add_children(es2panda_arg)
        found_change_type_link = Es2pandaLibApi.change_types.find do |x|
          x.es2panda_arg.type ==
            '|AstTypeAdditionalChildren|' && Arg.check_ptr_depth(x, es2panda_arg['type'].ptr_depth || 0)
        end
      end

      if Arg.is_ast_node(es2panda_arg)
        found_change_type_link = Es2pandaLibApi.change_types.find do |x|
          x.es2panda_arg.type == '|AstNode|' && Arg.check_ptr_depth(x, es2panda_arg['type'].ptr_depth || 0)
        end
      end

      if Arg.is_ast_node_add_children(es2panda_arg)
        found_change_type_link = Es2pandaLibApi.change_types.find do |x|
          x.es2panda_arg.type ==
            '|AstNodeAdditionalChildren|' && Arg.check_ptr_depth(x, es2panda_arg['type'].ptr_depth || 0)
        end
      end

      if Arg.is_var_type(es2panda_arg)
        found_change_type_link = Es2pandaLibApi.change_types.find do |x|
          x.es2panda_arg.type == '|Variable|' && Arg.check_ptr_depth(x, es2panda_arg['type'].ptr_depth || 0)
        end
      end

      if Arg.is_enum_type(es2panda_arg)
        found_change_type_link = Es2pandaLibApi.change_types.find do |x|
          x.es2panda_arg.type == '|Enum|' && Arg.check_ptr_depth(x, es2panda_arg['type'].ptr_depth || 0)
        end
      end

      if Arg.is_scope_type(es2panda_arg)
        found_change_type_link = Es2pandaLibApi.change_types.find do |x|
          x.es2panda_arg.type == '|Scope|' && Arg.check_ptr_depth(x, es2panda_arg['type'].ptr_depth || 0)
        end
      end

      if Arg.is_decl_type(es2panda_arg)
        found_change_type_link = Es2pandaLibApi.change_types.find do |x|
          x.es2panda_arg.type == '|Declaration|' && Arg.check_ptr_depth(x, es2panda_arg['type'].ptr_depth || 0)
        end
      end

      if Arg.is_code_gen(es2panda_arg)
        found_change_type_link = Es2pandaLibApi.change_types.find do |x|
          x.es2panda_arg.type == '|CodeGen|' && Arg.check_ptr_depth(x, es2panda_arg['type'].ptr_depth || 0)
        end
      end

      found_change_type_link
    end

    def self.check_is_type_supported(found_change_type_link, es2panda_arg)
      unless found_change_type_link || Es2pandaLibApi.primitive_types.include?(es2panda_arg['type'].name)
        return "Unsupported type: #{Arg.unsupported_type_msg(es2panda_arg)}"
      end

      if found_change_type_link && !Arg.check_ptr_depth(found_change_type_link, es2panda_arg['type']['ptr_depth'] || 0)
        return "Invalid ptr depth for type: #{Arg.unsupported_type_msg(es2panda_arg)}"
      end

      if found_change_type_link && es2panda_arg['name'] == 'returnType' &&
         !found_change_type_link.cast.respond_to?('reverse_cast')
        return "Unsupported return type: #{Arg.unsupported_type_msg(es2panda_arg)}"
      end

      if found_change_type_link && es2panda_arg['name'] == 'callType' &&
         !found_change_type_link.cast.respond_to?('call_cast')
        return "Unsupported call type: #{Arg.unsupported_type_msg(es2panda_arg)}"
      end

      if found_change_type_link && es2panda_arg['name'] == 'constructorType' &&
         !found_change_type_link.cast.respond_to?('constructor_cast')
        "Unsupported constructor type: #{Arg.unsupported_type_msg(es2panda_arg)}"
      end
    end

    def self.get_change_type_info_with_err_msg(es2panda_arg)
      found_change_type_link = Arg.get_change_type_info(es2panda_arg)
      err_msg = Arg.check_is_type_supported(found_change_type_link, es2panda_arg)
      [found_change_type_link, err_msg]
    end

    def set_default_expression_cast(found_change_type)
      return unless found_change_type && @is_optional && @default_optional
      found_change_type.cast['expression'] = found_change_type.cast['default_expression']
      found_change_type.new_args = found_change_type.default_new_args || []
    end

    def initialize(arg_info, base_namespace, default_optional = false)
      found_change_type_link, err_msg = Arg.get_change_type_info_with_err_msg(arg_info)
      raise err_msg unless err_msg.nil?

      @@primitive_types ||= Es2pandaLibApi.primitive_types
      @es2panda_arg = arg_info
      @const = Arg.const(arg_info)
      @es2panda_arg['const'] = @const
      @base_namespace = base_namespace
      @default_optional = default_optional

      @is_ast_node = Arg.is_ast_node(es2panda_arg)
      @is_ast_node_add_children = Arg.is_ast_node_add_children(es2panda_arg)
      @is_ast_type = Arg.is_ast_type(es2panda_arg)
      @is_ast_type_add_children = Arg.is_ast_type_add_children(es2panda_arg)
      @is_var_type = Arg.is_var_type(es2panda_arg)
      @is_enum_type = Arg.is_enum_type(es2panda_arg)
      @is_scope_type = Arg.is_scope_type(es2panda_arg)
      @is_decl_type = Arg.is_decl_type(es2panda_arg)
      @is_code_gen = Arg.is_code_gen(es2panda_arg)

      if found_change_type_link
        found_change_type = Marshal.load(Marshal.dump(found_change_type_link))
        @is_optional = found_change_type.es2panda_arg.respond_to?('optional') && found_change_type.es2panda_arg['optional']
        set_default_expression_cast(found_change_type)

        placeholders = find_placeholders(found_change_type.es2panda_arg)

        replacements = placeholders.map do |path, placeholder|
          value = get_value_by_path(@es2panda_arg, path)
          [placeholder, value]
        end

        found_change_type.es2panda_arg = Marshal.load(Marshal.dump(@es2panda_arg))

        if stop_modify_idl_arg
          found_change_type.idl_args = [Marshal.load(Marshal.dump(@es2panda_arg))]
        else
          found_change_type.idl_args = Marshal.load(Marshal.dump(found_change_type.new_args))
          found_change_type.idl_return_args = Marshal.load(Marshal.dump(found_change_type.return_args))
        end

        found_change_type = deep_replace(found_change_type, replacements)

        clever_replacements = []
        template_args = []

        if found_change_type&.new_args&.length && ((found_change_type.new_args.length > 1 &&
           @es2panda_arg['type'].respond_to?('template_args')) ||
           Es2pandaLibApi.additional_containers.include?(@es2panda_arg['type'].name))
          accessor = (@es2panda_arg['type']['ptr_depth'] || 0).zero? ? '.' : '->'
          clever_replacements += [['|accessor|', accessor]]

          @es2panda_arg['type']['template_args'].each_with_index do |template_arg_raw, i|
            template_arg = Arg.new({ 'name' => @es2panda_arg['name'] + "Element#{i + 1}",
                                     'type' => ClassData.add_base_namespace(template_arg_raw['type'], base_namespace) },
                                   base_namespace)

            raise "Unsupported double+ nested complex types: #{@es2panda_arg}\n" if template_arg.lib_args.length > 1

            unless template_arg_raw['type']['const'].nil?
              Arg.set_const_modifier(@lib_args, 'const')
              Arg.set_const_modifier(@idl_args, 'const')
            end

            template_args += [template_arg]
          end

          found_change_type&.new_args&.map! do |arg|
            modify_template_nested_arg(arg, base_namespace)
          end

          found_change_type&.return_args&.map! do |arg|
            modify_template_nested_arg(arg, base_namespace)
          end

          found_change_type&.idl_args&.map! do |arg|
            modify_template_nested_arg(arg, base_namespace, true)
          end

          found_change_type&.idl_return_args&.map! do |arg|
            modify_template_nested_arg(arg, base_namespace, true)
          end

          correct_depths(found_change_type)

          template_args.each_with_index do |template_arg, i|
            clever_replacements += [["|template_nested_expression_#{i + 1}|",
                                     if template_arg.lib_cast['expression']
                                       template_arg.lib_cast['expression'] + "\n" else '' end]]
            clever_replacements += [["|reverse_template_nested_expression_#{i + 1}_start|",
                                     template_arg.lib_cast['reverse_cast']&.start&.gsub(
                                       '?const?', template_arg.const || '') || '']]
            clever_replacements += [["|reverse_template_nested_expression_#{i + 1}_end|",
                                     (template_arg.lib_cast['reverse_cast']&.end || '')]]
            unless found_change_type.new_args[i].nil?
              found_change_type.new_args[i]['local_var_name'] = template_arg.lib_cast['var_name']
            end
          end

          @es2panda_arg['type']['ref_depth'] == 2 &&
            found_change_type.cast['var_name'] = 'std::move(' + found_change_type.cast['var_name'] + ')'
        end

        clever_placeholders = find_placeholders(found_change_type)
        clever_replacements += clever_placeholders.map do |_path, placeholder|
          if placeholder.include?('.')
            value = get_placeholder_value(found_change_type, placeholder)
            [placeholder, value]
          else
            is_known_replacement = clever_replacements.any? do |sub_array|
              sub_array[0] =~ /_nested_expression_|\|accessor\|/
            end
            raise "Unknown placeholder: #{placeholder}\n" unless is_known_replacement
          end
        end

        found_change_type = deep_replace(found_change_type, clever_replacements)

        @lib_args = found_change_type&.new_args
        @idl_args = found_change_type&.idl_args
        @lib_cast = found_change_type&.cast
        @return_args = found_change_type&.return_args
        @idl_return_args = found_change_type&.idl_return_args
        nested_arg_transform

      end

      if @lib_args.nil?
        @lib_args = check_allowed_type(@es2panda_arg) ? [@es2panda_arg] : []
        @idl_args = @lib_args
      end

      delete_refs(@lib_args)
      delete_refs(@idl_args)

      if @lib_cast.nil?
        @lib_cast = {}
        @lib_cast['var_name'] = @es2panda_arg['name']
      else
        @need_var_cast = true
      end

      return unless @es2panda_arg['name'] == 'returnType'

      Arg.set_const_modifier(@lib_args, @const)
      Arg.set_const_modifier(@idl_args, @const)
    end

    def check_allowed_type(arg)
      @@primitive_types.include?(arg['type'].name) ||
        @is_enum_type || @is_ast_node || @is_ast_node_add_children || @is_ast_type || @is_ast_type_add_children ||
        @is_code_gen || @is_scope_type || @is_var_type
    end

    def nested_arg_transform
      return unless @lib_args && @lib_args.size == 1 && !check_allowed_type(@lib_args[0])

      Arg.set_const_modifier(@lib_args, @const)
      Arg.set_const_modifier(@idl_args, @const)
      tmp = Arg.new(@lib_args[0], @base_namespace, @default_optional)
      @lib_args = tmp.lib_args
      @idl_args = tmp.idl_args
      @lib_cast = tmp.lib_cast
      @return_args = tmp.return_args
      @idl_return_args = tmp.idl_return_args
      @is_optional = tmp.is_optional
      return if tmp.check_allowed_type(@lib_args[0])

      nested_arg_transform
    end

    def find_placeholders(data, path = [])
      placeholders = []
      if data.is_a?(OpenStruct)
        data.each_pair do |key, value|
          placeholders += find_placeholders(value, path + [key.to_s])
        end
      elsif data.is_a?(Array)
        data.each_with_index do |value, index|
          placeholders += find_placeholders(value, path + [index])
        end
      elsif data.is_a?(String)
        data.scan(/\|(.+?)\|/) do |match|
          placeholders << [path.join('.'), '|' + match[0] + '|']
        end
      end
      placeholders
    end

    def get_value_by_path(data, path)
      path.split('.').reduce(data) do |current, key|
        current.is_a?(Array) ? current[key.to_i] : current[key]
      end
    end

    def get_placeholder_value(found_change_type, placeholder)
      path_to_value = placeholder.gsub('|', '').gsub('_int', '').gsub(' - 1', '')
      value = get_value_by_path(found_change_type, path_to_value) || 0
      if value.is_a?(Integer)
        if placeholder.end_with?('ptr_depth|')
          value = '*' * value
        elsif placeholder.end_with?('ref_depth|')
          value = '&' * value
        elsif placeholder.end_with?('ptr_depth_int|')
          value = value
        elsif placeholder.end_with?('ptr_depth - 1|')
          value = '*' * (value - 1)
        elsif placeholder.end_with?('namespace|')
          value = ''
        elsif placeholder.end_with?('const|')
          value = ''
        else
          raise "Unknown int found for placeholder '#{placeholder}', value: #{value}\n"
        end
      end
      value
    end

    # replacements = [[from1, to1], [from2, to2], ...]
    def deep_replace(data, replacements)
      if data.is_a?(OpenStruct)
        data.each_pair do |key, value|
          data[key] = deep_replace(value, replacements)
        end
      elsif data.is_a?(Array)
        data.map! { |value| deep_replace(value, replacements) }
      elsif data.is_a?(String)
        found_replacement = replacements.find { |first, _| first == data }
        if found_replacement
          data = if found_replacement[1].is_a?(String)
                   found_replacement[1]
                 else
                   found_replacement[1].dup
                 end
        else
          replacements.each { |from, to| data.gsub!(from, to) if to.is_a?(String) }
        end
      end
      data
    end

    def correct_depths(data)
      if data.is_a?(OpenStruct)
        if data.respond_to?(:increase_ptr_depth)
          ptr_depth = data.type.ptr_depth || 0
          ptr_depth += data.increase_ptr_depth.to_i
          data.type['ptr_depth'] = ptr_depth
          data.delete_field(:increase_ptr_depth)
        else
          data.each_pair do |key, value|
            data[key] = correct_depths(value)
          end
        end
      elsif data.is_a?(Array)
        data.map! { |value| correct_depths(value) }
      end
      data
    end

    attr_reader :es2panda_arg
    attr_reader :lib_args
    attr_reader :idl_args
    attr_reader :lib_cast
    attr_reader :return_args
    attr_reader :idl_return_args
    attr_reader :const
    attr_reader :is_optional

    def lib_args_to_str
      @lib_args.map do |lib_arg|
        Arg.arg_to_str(lib_arg)
      end&.join(', ')
    end

    def lib_args_to_idl
      @idl_args.map do |idl_arg|
        Arg.arg_to_idl(idl_arg)
      end.join(', ')
    end

    def self.arg_value(arg)
      ptr_depth = arg['type']['ptr_depth'] || 0
      '*' * ptr_depth + arg['name']
    end

    def self.arg_to_str(arg)
      type_to_str(arg['type']) + arg['name']
    end

    def self.type_to_str(type)
      res = type['const'] ? "#{type['const']} " : ''
      res += type['name']
      ptr_depth = type['ptr_depth'] || 0
      ref_depth = type['ref_depth'] || 0
      "#{res} #{'*' * ptr_depth}#{'&' * ref_depth}"
    end

    def self.arg_to_idl(arg)
      name = arg['name']
      name += '_arg' if %w[optional object readonly sequence].include?(name)
      "#{type_to_idl(arg['type'])} #{name}"
    end

    def self.type_to_idl(type)
      @@replace_to_idl_types ||= {
        'bool' => 'boolean',
        'int' => 'i32',
        'size_t' => 'u32',
        'char' => 'i8',
        'int8_t' => 'i8',
        'uint8_t' => 'u8',
        'int16_t' => 'i16',
        'char16_t' => 'i16',
        'int32_t' => 'i32',
        'uint32_t' => 'u32',
        'int64_t' => 'i64',
        'uint64_t' => 'u64',
        'float' => 'f32',
        'double' => 'f64',
        'sequence<i8>' => 'String'
      }

      c_type = type['namespace'] && type['namespace'] != 'std' ? "#{type['namespace']}.#{type['name']}" : type['name']
      res_sequence_len = @@replace_to_idl_types.key?(c_type) ? (type['ptr_depth'] || 0) : ((type['ptr_depth'] || 1) - 1)
      res = res_sequence_len.times.reduce(c_type) { |acc, _| "sequence<#{@@replace_to_idl_types[acc] || acc}>" }
      @@replace_to_idl_types[res] || res
    end

    attr_reader :is_change_type

    def updater_allowed
      @is_ast_node
    end
  end

  class Type
    @raw_type = nil
    @lib_type = nil
    @return_args = nil
    @idl_return_args = nil
    @cast = nil
    @usage = 'no'
    @base_namespace = ''

    def initialize(type_info, base_namespace, usage)
      @raw_type = type_info
      @base_namespace = base_namespace
      @usage = usage
      tmp_arg = Arg.new({ 'name' => "#{usage}Type", 'type' => @raw_type }, base_namespace)

      @return_args = tmp_arg.return_args
      @idl_return_args = tmp_arg.idl_return_args
      unless tmp_arg.lib_args.nil? || tmp_arg.lib_args[0].nil? || tmp_arg.lib_args[0]['type'].nil?
        @lib_type = tmp_arg.lib_args[0]['type']
        @idl_type = tmp_arg.idl_args[0]['type']
        remove_unused_const_modifier
      end
      @cast = tmp_arg.lib_cast
    end

    attr_reader :usage

    def remove_unused_const_modifier
      @lib_type['const'] = nil if const && !(@lib_type&.respond_to?('ptr_depth') && @lib_type&.ptr_depth&.positive?)
      @idl_type['const'] = nil if const && !(@idl_type&.respond_to?('ptr_depth') && @idl_type&.ptr_depth&.positive?)
    end

    def lib_type_to_str
      Arg.type_to_str(@lib_type)
    end

    def idl_type_to_str
      Arg.type_to_idl(@idl_type)
    end

    def arena_item_type
      type = Marshal.load(Marshal.dump(@lib_type))
      type['ptr_depth'] -= 1
      tmp_arg = Arg.new({ 'name' => '', 'type' => type }, @base_namespace)
      Arg.type_to_str(tmp_arg.lib_args[0]['type'])
    end

    def call_cast
      if @cast && usage.include?('call')
        if @cast['call_cast']['call_var']
          @cast['call_cast']['call_var_str'] = Arg.arg_to_str(@cast['call_cast']['call_var'])
        end

        @cast['call_cast']
      else
        ''
      end
    end

    def constructor_cast
      if @cast && usage.include?('constructor')
        @cast['constructor_cast']
      else
        ''
      end
    end

    def cast
      if @cast && @cast && usage.include?('return')
        @cast['reverse_cast']
      else
        ''
      end
    end

    def return_args_to_str
      @return_args ? @return_args.map { |arg| ", #{Arg.arg_to_str(arg)}" }.join('') : ''
    end

    def return_args_to_idl
      return '' unless @idl_return_args

      @idl_return_args
        .reject { |arg| arg['name'] == 'returnTypeLen' }
        .map { |arg| ", #{Arg.arg_to_idl(arg)}" }
        .join('')
    end

    def const
      @lib_type['const']
    end

    attr_reader :raw_type
    attr_reader :lib_type
    attr_reader :idl_type
  end

  class ClassData < SimpleDelegator
    @class_base_namespace = ''
    @extends_classname = nil
    @template_extends = []
    @ast_node_type_value = nil

    attr_accessor :class_base_namespace
    attr_accessor :extends_classname
    attr_accessor :template_extends
    attr_accessor :ast_node_type_value

    def class_name
      Es2pandaLibApi.classes&.each do |_namespace_name, namespace_classes|
        if namespace_classes&.find { |_name, data| data == self }
          return namespace_classes&.find { |_name, data| data == self }[0]
        end
      end
      if Es2pandaLibApi.structs.find { |_name, data| data == self }[0]
        return Es2pandaLibApi.structs.find { |_name, data| data == self }[0]
      end

      raise 'Unknown class name'
    end

    def class_c_type
      change_type_info, err_msg = Arg.get_change_type_info_with_err_msg(
        {
          'name' => 'class_c_type',
          'type' => OpenStruct.new({ 'name' => class_name, 'ptr_depth' => 1, 'namespace' => @class_base_namespace })
        }
      )

      return '' if err_msg

      c_type = change_type_info.dig('new_args', 0, 'type', 'name')
      ", c_type=#{c_type}" unless c_type.nil? # In some cases new_args = []. E.g. Checker type.
    end

    def call_cast
      name = class_name
      class_type = Type.new(add_base_namespace(OpenStruct.new({ 'name' => name, 'ptr_depth' => 1 })),
                            class_base_namespace, 'call')
      class_type.call_cast
    end

    def constructor_type
      name = class_name
      Type.new(add_base_namespace(OpenStruct.new({ 'name' => name, 'ptr_depth' => 1 })), class_base_namespace,
               'constructor')
    end

    def constructor_cast
      name = class_name
      class_type = Type.new(add_base_namespace(OpenStruct.new({ 'name' => name, 'ptr_depth' => 1 })),
                            class_base_namespace, 'constructor')
      class_type.constructor_cast
    end

    def updater_allowed
      name = class_name
      class_arg = Arg.new(OpenStruct.new({ 'name' => 'empty',
                                           'type' =>
                                           add_base_namespace(OpenStruct.new({ 'name' => name, 'ptr_depth' => 1 })) }),
                          class_base_namespace)
      class_arg.updater_allowed
    end

    def usings_map
      dig(:usings)&.map { |using| [using['name'], using['type']] }.to_h || {}
    end

    def check_no_replace_using(_type, new_type)
      res = false
      Es2pandaLibApi.no_usings_replace_info['using types']&.each do |using|
        res ||= Es2pandaLibApi.check_fit(new_type, using)
        res ||= Es2pandaLibApi.check_fit(_type, using)
      end
      !res
    end

    def replace_with_usings(type, usings)
      if usings[type&.name]
        new_type = usings[type.name]
        return type unless check_no_replace_using(type, new_type)

        new_type['ref_depth'] = type['ref_depth'] || 0
        new_type['ptr_depth'] = type['ptr_depth'] || 0
        return new_type
      end
      type
    end

    def add_base_namespace(type)
      ClassData.add_base_namespace(type, class_base_namespace)
    end

    def self.add_base_namespace(type, base_namespace)
      type['namespace'] ||= base_namespace unless Es2pandaLibApi.primitive_types.include?(type['name'])
      type
    end

    def error_catch_log(mode, function, err, func_new_name)
      Es2pandaLibApi.stat_add_unsupported_type(err.message) if err.message.include?('Unsupported type')
      if mode == 'struct_getter'
        Es2pandaLibApi.log('warning', "Error: '#{err.message}'")
        Es2pandaLibApi.log('debug', "Field name: #{function.name}\nField type:\n---\n"\
                                                                "#{function.type}\n---\n\n")
        Es2pandaLibApi.log('backtrace', err.backtrace.join("\n"), "\n\n")
        Es2pandaLibApi.stat_add_method(0, class_name, class_base_namespace, func_new_name)
        return
      end

      Es2pandaLibApi.log('warning', "#{err.message}\n")
      Es2pandaLibApi.log('debug', "#{mode} name: #{function.name}\n\n"\
                                    "#{mode} raw C++ declaration:\n"\
                                    "```\n#{function.raw_declaration}\n```\n")
      Es2pandaLibApi.log('backtrace', err.backtrace.join("\n"), "\n\n")

      if mode == 'Constructor'
        Es2pandaLibApi.stat_add_constructor(0, class_name, class_base_namespace, func_new_name)
        Es2pandaLibApi.stat_add_class(0, function.name)
      elsif mode == 'Method'
        Es2pandaLibApi.stat_add_method(0, class_name, class_base_namespace, func_new_name)
      else
        raise 'Unreachable'
      end
    end

    def class_constructors
      res = []
      usings = usings_map
      constructor_overload = {}
      idl_constructor_overload = {}
      constructors = dig(:constructors)
      constructors_with_optional = []
      [constructors, constructors_with_optional].each do |constructors_list|
        is_with_optional_list = constructors_list == constructors_with_optional
        constructors_list&.each do |constructor|
          if check_no_gen_constructor(constructor)
            begin
              args = []
              constructor_cast
              optional_args_info = []
              constructor.args&.each do |arg|
                arg['type'] = replace_with_usings(arg['type'], usings)
                arg['type'] = add_base_namespace(arg['type'])
                arg['type']['current_class'] = constructor.name
                processed_arg = Arg.new(arg, class_base_namespace, !is_with_optional_list)
                if processed_arg.is_optional && !is_with_optional_list
                  optional_args_info << { name: arg['type']['name'] }
                end
                args << processed_arg
              end
              if !optional_args_info.empty? && !is_with_optional_list
                constructors_with_optional << Marshal.load(Marshal.dump(constructor))
              end
            rescue StandardError => e
              error_catch_log('Constructor', constructor, e, 'Create' + class_name)
            else
              Es2pandaLibApi.stat_add_constructor(1, class_name, class_base_namespace, 'Create' + class_name)
              Es2pandaLibApi.stat_add_class(1, class_name)

              Es2pandaLibApi.log('info', "Supported constructor for class '#{class_name}'\n")
              res << { 'overload' => get_new_method_name(constructor_overload, '', ''),
                      'idl_overload' => get_new_method_name(idl_constructor_overload, '', '', true),
                      'args' => args, 'raw_decl' => constructor.raw_declaration }
            end
          else
            Es2pandaLibApi.log('info', "Banned constructor for class '#{class_name}'\n")
          end
        end
      end
      res
    end

    def check_no_gen_constructor(constructor)
      Es2pandaLibApi.ignored_info['postfix_contains']&.each do |postfix|
        return false if constructor.postfix&.include?(postfix)
      end
      constructor.args&.each do |arg|
        Es2pandaLibApi.ignored_info['args']&.each do |banned_arg_pattern|
          return false if Es2pandaLibApi.check_fit(arg, banned_arg_pattern)
        end
        return false unless Es2pandaLibApi.check_template_type_presents(arg.type)
      end
      Es2pandaLibApi.ignored_info['constructors']['call_class']&.each do |call_class|
        return false if call_class['name'] == class_name
      end
      Es2pandaLibApi.ignored_info['template_names']&.each do |template_name|
        return false if constructor.respond_to?('template') && constructor['template'].include?(template_name['name'])
      end
      # NOTE(nikozer) Need to ban ast verifier default condtructor, will be removed later
      return false if class_name == 'ASTVerifier' && constructor.args&.size == 0

      true
    end

    def check_no_gen_method(method)
      return true if Es2pandaLibApi.allowed_info[class_name]&.include?(method.name)
      return false unless method['template'].nil?

      Es2pandaLibApi.ignored_info['postfix_contains']&.each do |postfix|
        return false if method.postfix&.include?(postfix)
      end
      method.args&.each do |arg|
        Es2pandaLibApi.ignored_info['args']&.each do |banned_arg_pattern|
          return false if Es2pandaLibApi.check_fit(arg, banned_arg_pattern)
        end
        return false unless Es2pandaLibApi.check_template_type_presents(arg.type)
      end
      Es2pandaLibApi.ignored_info['return_type']&.each do |return_type|
        return false if method.return_type['name'] == return_type['name'] && (!return_type.respond_to?('namespace') ||
                            return_type['namespace'] == method.return_type['namespace'])
      end
      Es2pandaLibApi.ignored_info['methods']['call_class']&.each do |call_class|
        return false if call_class['name'] == class_name
      end
      return false unless Es2pandaLibApi.check_template_type_presents(method.return_type)

      Es2pandaLibApi.ignored_info['template_names']&.each do |template_name|
        return false if method.respond_to?('template') && method['template'].include?(template_name['name'])
      end
      true
    end

    def check_no_get_field(field)
      Es2pandaLibApi.ignored_info['return_type']&.each do |return_type|
        return false if field.type['name'] == return_type['name'] && (!return_type.respond_to?('namespace') ||
        return_type['namespace'] == field.type['namespace'])
      end
      Es2pandaLibApi.ignored_info['methods']['call_class']&.each do |call_class|
        return false if call_class['name'] == class_name
      end
      return false unless Es2pandaLibApi.check_template_type_presents(field.type)

      true
    end

    def get_return_expr(return_type, call_cast, consts, method, args, function_type)
      raise 'Unsupported function type' unless %w[method struct_getter].include?(function_type)

      return_expr = ''
      const, const_return = consts

      if return_type.raw_type['name'] != 'void' && !((return_type.return_args_to_str &&
         !return_type.return_args_to_str.empty?) ||
         Es2pandaLibApi.additional_containers.include?(return_type.raw_type['name']))
        return_expr += 'auto apiRes = '
      end

      return_expr += return_type.cast['start']&.gsub('?const?', const_return) if return_type.cast

      if function_type == 'method'
        return_expr += '(' + call_cast['start']&.gsub('?const?', const) + method['name'] + '('
        return_expr += args&.map do |arg|
          arg.lib_cast['var_name'] if arg.lib_cast
        end&.join(', ')
      elsif function_type == 'struct_getter'
        return_expr += '(' + call_cast['start']&.gsub('?const?', const) + method['name']
      end

      return_expr += ')' + (call_cast['end']&.gsub('?const?', const) || '') + ')'

      return_expr += return_type.cast['end']&.gsub('?const?', const_return) || '' if return_type.cast

      return_expr += if return_type.raw_type['name'] == 'void'
                       ';'
                     else
                       ";\n\treturn apiRes;"
                     end
      return_expr
    end

    def check_for_same_class_name
      res = false
      class_name_for_check = class_name
      Es2pandaLibApi.classes&.each do |_namespace_name, namespace_classes|
        res |= namespace_classes&.find { |_name, data| data != self && _name == class_name_for_check }
      end
      res
    end

    def get_new_method_name(function_overload, name, const, skip_namespace_overload = false)
      function_name = if check_for_same_class_name && !skip_namespace_overload then class_base_namespace.capitalize
                      else '' end + name + (const != '' ? 'Const' : '')
      overload_name = function_name

      if function_overload.include?(function_name)
        overload_name += function_overload[function_name].to_s
        function_overload[function_name] += 1
      else
        function_overload[function_name] = 1
      end
      overload_name
    end

    def get_const_return_modifier(return_type)
      if return_type&.const && return_type&.const != '' && return_type&.lib_type&.respond_to?('ptr_depth') &&
         return_type&.lib_type&.ptr_depth&.positive?
        'const'
      else
        ''
      end
    end

    def get_const_modifier(method)
      if method.respond_to?('postfix') && method.postfix.include?('const')
        'const'
      else
        ''
      end
    end

    def class_methods
      res = []
      function_overload = {}
      idl_function_overload = {}
      usings = usings_map
      methods = dig(:methods)
      return res unless methods

      template_extends.each do |template_extend|
        methods += Es2pandaLibApi.classes['ir'][template_extend]['methods']
      end
      methods_with_optional = []
      [methods, methods_with_optional].each do |methods_list|
        is_with_optional_list = methods_list == methods_with_optional
        methods_list&.each do |method|
          if check_no_gen_method(method)
            begin
              return_type = Type.new(add_base_namespace(replace_with_usings(method.return_type, usings)),
                                    class_base_namespace, 'return')
              const = get_const_modifier(method)
              const_return = get_const_return_modifier(return_type)

              args = []
              optional_args_info = []
              method.args.each_with_index do |arg, arg_index|
                arg['type'] = add_base_namespace(replace_with_usings(arg['type'], usings))
                processed_arg = Arg.new(arg, class_base_namespace, !is_with_optional_list)
                if processed_arg.is_optional && !is_with_optional_list
                  optional_args_info << { name: arg['type']['name'] }
                end
                args << processed_arg
              end

              if !optional_args_info.empty? && !is_with_optional_list
                methods_with_optional << Marshal.load(Marshal.dump(method))
              end

              return_expr = get_return_expr(return_type, call_cast, [const, const_return], method, args, 'method')
            rescue StandardError => e
              stat_function_overload = Marshal.load(Marshal.dump(function_overload))
              error_catch_log('Method', method, e, class_name +
              get_new_method_name(stat_function_overload, method.name, const))
            else
              stat_function_overload = Marshal.load(Marshal.dump(function_overload))
              Es2pandaLibApi.stat_add_method(1, class_name, class_base_namespace, class_name +
              get_new_method_name(stat_function_overload, method.name, const))
              Es2pandaLibApi.log('info', 'supported method: ', method.name, ' class: ', class_name, "\n")

              res << { 'name' => method.name, 'const' => const, 'return_arg_to_str' => return_type.return_args_to_str,
                      'overload_name' => get_new_method_name(function_overload, method.name, const), 'args' => args,
                      'idl_name' => get_new_method_name(idl_function_overload, method.name, const, true),
                      'return_type' => return_type, 'return_expr' => return_expr, 'raw_decl' => method.raw_declaration,
                      'const_return' => const_return, 'get_modifier' => method['additional_attributes'] }
            end
          else
            Es2pandaLibApi.log('info', "Banned method\n")
          end
        end
      end
      res
    end

    def struct_getters
      res = []
      usings = usings_map
      dig(:members)&.each do |field|
        if check_no_get_field(field)
          begin
            return_type = Type.new(add_base_namespace(replace_with_usings(field.type, usings)),
                                   class_base_namespace, 'return')
            const = ''
            const_return = get_const_return_modifier(return_type)

            return_expr = get_return_expr(return_type, call_cast, [const, const_return], field, nil, 'struct_getter')
          rescue StandardError => e
            error_catch_log('struct_getter', field, e, class_name + field.name)
          else
            Es2pandaLibApi.stat_add_method(1, class_name, class_base_namespace, class_name + field.name)
            Es2pandaLibApi.log('info', 'supported struct getter: ', field.name, ' struct: ', class_name, "\n")

            res << { 'name' => field.name, 'return_arg_to_str' => return_type.return_args_to_str,
                     'return_type' => return_type, 'return_expr' => return_expr, 'const_return' => const_return }
          end
        else
          Es2pandaLibApi.log('info', "Banned method\n")
        end
      end
      res
    end
  end

  @ast_nodes = Set.new(%w[AstNode Expression Statement TypeNode])
  @ast_node_mapping = []
  @ast_types = Set.new(['Type'])
  @scopes = Set.new(%w[Scope VariableScope])
  @declarations = Set.new(['Decl'])
  @classes = {}
  @structs = {}
  @includes = Set.new
  @change_types = []
  @ignored_info = {}
  @allowed_info = {}
  @enums = Set.new(%w[AstNodeType ScopeType DeclType])

  @all_methods = 0.0
  @all_constructors = 0.0
  @all_classes = Set.new
  @classes_with_supported_constructor = Set.new
  @unsupported_types = {}

  @supported_methods = 0.0
  @supported_method_type = {}
  @supported_constructors = 0.0
  @supported_constructor_type = {}

  @info_log = false
  @debug_log = false
  # 29204
  @warning_log = false
  @backtrace_log = false
  @stat_log = false
  @type_stat_log = false

  def log(debug_level, *args)
    if debug_level == 'info' && @info_log
      print args.join('').to_s
    elsif debug_level == 'debug' && @debug_log
      print args.join('').to_s
    elsif debug_level == 'warning' && @warning_log
      print "WARNING:[e2p_api_generator]: #{args.join('')}"
    elsif debug_level == 'backtrace' && @backtrace_log
      print args.join('').to_s
    elsif debug_level == 'stat' && @stat_log
      print args.join('').to_s
    elsif debug_level == 'type_stat' && @type_stat_log
      print args.join('').to_s
    end
  end

  def no_usings_replace_info
    { 'using types' => [
      { 'name' => 'function', 'namespace' => 'std' },
      { 'name' => 'ExternalSource', 'namespace' => 'parser' },
      { 'name' => 'DirectExternalSource', 'namespace' => 'parser' },
      { 'name' => 'ETSNolintsCollectionMap', 'namespace' => 'parser' }
    ] }
  end

  def check_fit(data, pattern)
    return true if data.nil? || pattern.nil?

    if pattern.is_a?(OpenStruct) || pattern.is_a?(Hash)
      pattern.each_pair do |key, value|
        return false unless check_fit(data[key], value)
      end
    elsif pattern.is_a?(Array)
      pattern.each_with_index do |value, i|
        return false unless check_fit(data[i], value)
      end
    elsif pattern.is_a?(String) || pattern.is_a?(Integer)
      return false if data != pattern
    end

    true
  end

  def check_template_type_presents(type)
    type['template_args']&.each do |template_arg|
      return false unless Es2pandaLibApi.allowed_info['template_types'].include?(template_arg.type.name)

      if template_arg.type.respond_to?('template_args')
        Es2pandaLibApi.log('warning', 'Unexpected nested template args in type!')
        return false
      end
    end
    true
  end

  def check_class_type(class_name, class_base_namespace)
    type = ''
    if ast_nodes.include?(class_name) && class_base_namespace == 'ir'
      type = 'AST manipulation'
    elsif ast_node_additional_children.include?(class_name) && class_base_namespace == 'ir'
      type = 'AST manipulation'
    elsif class_base_namespace == 'ir'
      type = 'AST manipulation'
    elsif ast_types.include?(class_name) && class_base_namespace == 'checker'
      type = 'Type manipulation'
    elsif ast_type_additional_children.include?(class_name) && class_base_namespace == 'checker'
      type = 'Type manipulation'
    elsif class_base_namespace == 'checker'
      type = 'Type check'
    elsif ast_variables.find { |x| x[1] == class_name } && class_base_namespace == 'varbinder'
      type = 'Variable manipulation'
    elsif scopes.include?(class_name) && class_base_namespace == 'varbinder'
      type = 'Scope manipulation'
    elsif declarations.include?(class_name) && class_base_namespace == 'varbinder'
      type = 'Declaration'
    elsif class_base_namespace == 'varbinder'
      type = 'Varbinder manipulation'
    elsif class_base_namespace == 'parser'
      type = 'Parser manipulation'
    elsif class_base_namespace == 'compiler::ast_verifier'
      type = 'AST Verifier functions'
    elsif class_base_namespace == 'util'
      type = 'Import path manager'
    elsif class_base_namespace == 'gen'
      type = 'Compiler options'
    elsif class_base_namespace == 'es2panda'
      type = 'Arkts config'
    else
      raise 'Unsupported class type for stats class name: "' +
            class_name + '" class namespace: "' + class_base_namespace + '"'
    end
    type
  end

  def stat_add_method_type(class_name, class_base_namespace, func_new_name)
    type = check_class_type(class_name, class_base_namespace)
    if @supported_method_type[type]
      @supported_method_type[type][0] += 1
      @supported_method_type[type][1] << func_new_name
    else
      @supported_method_type[type] = [1, [func_new_name]]
    end
  end

  def stat_add_constructor_type(class_name, class_base_namespace, constructor_new_name)
    type = check_class_type(class_name, class_base_namespace)
    if @supported_constructor_type[type]
      @supported_constructor_type[type][0] += 1
      @supported_constructor_type[type][1] << constructor_new_name
    else
      @supported_constructor_type[type] = [1, [constructor_new_name]]
    end
  end

  def stat_add_method(support, class_name, class_base_namespace, func_new_name)
    @all_methods += 1
    @supported_methods += support
    stat_add_method_type(class_name, class_base_namespace, func_new_name) if support == 1
  end

  def stat_add_constructor(support, class_name, class_base_namespace, constructor_new_name)
    @all_constructors += 1
    @supported_constructors += support
    stat_add_constructor_type(class_name, class_base_namespace, constructor_new_name) if support == 1
  end

  def stat_add_class(support, class_name)
    @all_classes << class_name
    @classes_with_supported_constructor << class_name if support != 0
  end

  def stat_add_unsupported_type(err_msg)
    @unsupported_types[err_msg] ||= 0
    @unsupported_types[err_msg] += 1
  end

  def print_stats
    Es2pandaLibApi.log('stat',
    "--------------\n"\
    "Es2panda API generated:\n"\
    " - Methods: #{@supported_methods} / #{@all_methods} ( #{(@supported_methods / @all_methods * 100).round(2)} % )\n"\
    " - Constructors: #{@supported_constructors} / #{@all_constructors} ( "\
    "#{(@supported_constructors / @all_constructors * 100).round(2)} % )\n"\
    " - Classes: #{@classes_with_supported_constructor.size} / #{@all_classes.size} ( "\
    "#{(@classes_with_supported_constructor.size.to_f / @all_classes.size * 100).round(2)} % )\n"\
    "--------------\n")
  end

  def ast_variables
    [%w[NO Variable],
     %w[LOCAL LocalVariable],
     %w[GLOBAL GlobalVariable],
     %w[MODULE ModuleVariable],
     %w[ENUM EnumVariable]]
  end

  def ast_type_additional_children
    %w[
      ETSStringType
      ETSAsyncFuncReturnType
      ETSEnumType
      ETSBigIntType
    ]
  end

  def additional_containers
    %w[
      shared_ptr
    ]
  end

  def additional_classes_to_generate
    %w[
      TypeRelation
      GlobalTypesHolder
      Checker
      ETSChecker
      CheckerContext
      ResolveResult
      SignatureInfo
      Signature
      VarBinder
      ETSBinder
      RecordTable
      BoundContext
      ETSParser
      ASTVerifier
      CheckMessage
      Program
      ImportPathManager
      Options
      ArkTsConfig
      Path
    ]
  end

  def ast_node_additional_children
    %w[
      TypedStatement
      ClassElement
      AnnotatedExpression
      Literal
      LoopStatement
      MaybeOptionalExpression
      Property
    ]
  end

  def code_gen_children
    %w[
      CodeGen
      PandaGen
      ETSGen
    ]
  end

  def template_extends_classes
    %w[Annotated Typed AnnotationAllowed]
  end

  def primitive_types
    %w[
      char
      char16_t
      short
      int
      long
      long long
      float
      double
      long double
      bool
      void
      size_t
      uint8_t
      uint32_t
      uint64_t
      int8_t
      int16_t
      int32_t
      int64_t
      es2panda_Program
      es2panda_ExternalSource
      es2panda_AstNode
      es2panda_FunctionSignature
      es2panda_SourcePosition
      es2panda_SourceRange
      es2panda_SrcDumper
      es2panda_AstDumper
      es2panda_LabelPair
      es2panda_ScriptFunctionData
      es2panda_ImportSource
      es2panda_Signature
      es2panda_SignatureInfo
      es2panda_CheckerContext
      es2panda_ResolveResult
      es2panda_ValidationInfo
      es2panda_Type
      es2panda_TypeRelation
      es2panda_IndexInfo
      es2panda_GlobalTypesHolder
      es2panda_ObjectDescriptor
      es2panda_Variable
      es2panda_Scope
      es2panda_Declaration
      es2panda_RecordTable
      es2panda_AstVisitor
      es2panda_CodeGen
      es2panda_VReg
      NodeTraverser
      NodeTransformer
      NodePredicate
      es2panda_Config
      es2panda_Context
      PropertyProcessor
      PropertyTraverser
      ClassBuilder
      MethodBuilder
      ClassInitializerBuilder
      es2panda_variantDoubleCharArrayBool
      es2panda_variantIndex
      es2panda_DynamicImportData
      es2panda_IRNode
      es2panda_ScopeFindResult
      es2panda_BindingProps
      es2panda_BoundContext
      es2panda_ErrorLogger
      es2panda_ArkTsConfig
      es2panda_VerificationContext
      es2panda_AstVerifier
      es2panda_VerifierMessage
      es2panda_ImportPathManager
      es2panda_Options
      es2panda_Path
      es2panda_OverloadInfo
      Es2pandaLanguage
    ]
  end

  attr_reader :ast_nodes
  attr_reader :ast_types
  attr_reader :scopes
  attr_reader :declarations
  attr_reader :classes
  attr_reader :structs
  attr_reader :includes
  attr_reader :enums
  attr_reader :change_types
  attr_reader :ignored_info
  attr_reader :allowed_info

  def deep_to_h(object)
    case object
    when OpenStruct
      object.to_h.transform_keys(&:to_s).transform_values { |value| deep_to_h(value) }
    when Array
      object.map { |element| deep_to_h(element) }
    else
      object
    end
  end

  def extends_to_idl(extends)
    return nil unless extends && !extends.include?(',')

    if extends.include?('std::')
      Es2pandaLibApi.log('warning', "Unsupported inheritance from '#{extends}'\n")
      return nil
    end

    extends.gsub(/[<>]/, ' ').split.last.split('::').last
  end

  def template_extends(extends)
    res = []
    return res unless extends

    template_extends_classes.each do |extend|
      res << extend if extends.include?(extend)
    end
    res
  end

  def extract_classes_in_namespace(data, namespace_name, &is_class_needed)
    @classes[namespace_name] = {} unless @classes[namespace_name]
    data[namespace_name]&.class_definitions&.each do |class_definition|
      if is_class_needed.call(class_definition.name)
        class_data = ClassData.new(class_definition&.public)
        class_data.class_base_namespace = namespace_name
        class_data.extends_classname = extends_to_idl(class_definition.extends)
        class_data.template_extends = []
        @classes[namespace_name][class_definition.name] = class_data
      end
    end
  end

  def wrap_data(data)
    return unless data

    data.macros&.each do |macros|
      case macros.name
      when 'AST_NODE_MAPPING'
        @ast_nodes.merge(Set.new(macros.values.map { |x| x[1] }))
        @ast_node_mapping.concat(macros.values)
      when 'AST_NODE_REINTERPRET_MAPPING'
        @ast_nodes.merge(Set.new(macros.values.flat_map { |x| x[2..3] }))
        @ast_node_mapping.concat(macros.values.flat_map { |x| [[x[0], x[2]], [x[1], x[3]]] })
      when 'TYPE_MAPPING'
        @ast_types.merge(Set.new(macros.values.flat_map { |x| x[1] }))
      end
    end

    data.varbinder&.macros&.each do |macros|
      case macros.name
      when 'SCOPE_TYPES'
        @scopes.merge(Set.new(macros.values.map { |x| x[1] }))
      when 'DECLARATION_KINDS'
        @declarations.merge(Set.new(macros.values.map { |x| x[1] }))
      end
    end

    data.ast_node_reinterpret_mapping&.each do |mapping|
      @ast_nodes << mapping[2]
      @ast_nodes << mapping[3]
    end

    data.paths&.each do |include|
      @includes << include
    end

    data.change_types&.each do |change_type|
      @change_types << change_type
    end

    @ignored_info = deep_to_h(data.ignored_list) if data.ignored_list
    @allowed_info = deep_to_h(data.allowed_list) if data.allowed_list

    data.enums&.each do |enum|
      @enums << enum.name
    end
    Enums.wrap_data(data)

    @classes['ir'] = {} unless @classes['ir']
    data['ir']&.class_definitions&.each do |class_definition|
      class_data = ClassData.new(class_definition&.public)
      class_data.class_base_namespace = 'ir'
      class_data.extends_classname = extends_to_idl(class_definition.extends)
      class_data.template_extends = template_extends(class_definition.extends)
      flag_name = @ast_node_mapping.find { |elem| elem[1] == class_definition.name }&.first
      class_data.ast_node_type_value = Enums.get_astnodetype_value(flag_name)
      next if class_definition.name == 'NumberLiteral'  # it is defined manually

      @classes['ir'][class_definition.name] = class_data
    end

    extract_classes_in_namespace(data, 'checker') { |class_name|
        @ast_types.include?(class_name) || ast_type_additional_children.include?(class_name) ||
          additional_classes_to_generate.include?(class_name)
    }
    extract_classes_in_namespace(data, 'varbinder') { |class_name|
      scopes.include?(class_name) || declarations.include?(class_name) ||
        ast_variables.find { |x| x[1] == class_name } || additional_classes_to_generate.include?(class_name)
    }
    extract_classes_in_namespace(data, 'parser') { |class_name| additional_classes_to_generate.include?(class_name) }
    extract_classes_in_namespace(data, 'util') { |class_name| additional_classes_to_generate.include?(class_name) }
    extract_classes_in_namespace(data, 'gen') { |class_name| additional_classes_to_generate.include?(class_name) }
    extract_classes_in_namespace(data, 'es2panda') { |class_name| additional_classes_to_generate.include?(class_name) }

    @classes['ast_verifier'] = {} unless @classes['ast_verifier']
    data['ast_verifier']&.class_definitions&.each do |class_definition|
      if additional_classes_to_generate.include?(class_definition.name)
        class_data = ClassData.new(class_definition&.public)
        class_data.class_base_namespace = 'compiler::ast_verifier'
        class_data.template_extends = []
        @classes['ast_verifier'][class_definition.name] = class_data
      end
    end
  end

  module_function :wrap_data, :classes, :ast_nodes, :includes, :change_types, :enums, :ast_types, :check_fit, :log,
                  :stat_add_constructor, :stat_add_method, :print_stats, :ignored_info, :allowed_info, :primitive_types,
                  :stat_add_class, :stat_add_unsupported_type, :ast_node_additional_children, :code_gen_children,
                  :additional_classes_to_generate, :ast_type_additional_children, :scopes, :ast_variables, :deep_to_h,
                  :no_usings_replace_info, :declarations, :check_template_type_presents, :structs,
                  :additional_containers, :stat_add_constructor_type, :stat_add_method_type, :check_class_type,
                  :extends_to_idl, :template_extends, :template_extends_classes, :extract_classes_in_namespace
end

def Gen.on_require(data)
  Es2pandaLibApi.wrap_data(data)
end
