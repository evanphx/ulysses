syscalls = {}

class Syscall
  def initialize(num, name, args)
    @num = num
    @name = name
    @args = args
  end

  attr_reader :num, :name, :args

  REGS = %w!ebx ecx edx esi edi! #` %w!edi esi edx ecx ebx!

  def arg_types
    @args.map { |arg|
      arg == "..." ? arg : arg.split(/\s+/)[0..-2].join(" ")
    }
  end

  def decl_args
    arg_types.join(", ")
  end

  def num_args
    @args.size
  end

  def arg_regs
    REGS[0, num_args].zip(arg_types).map { |i,t|
      t == "..." ? "regs->#{i}" : "(#{t})regs->#{i}"
    }.join(", ")
  end

  def raw?
    @args.size == 1 and @args[0] == "Registers* regs"
  end
end

File.open File.expand_path("../syscall.cpp", __FILE__) do |f|
  f.each_line do |l|
    if m = /^\s*SYSCALL\((\d+),\s*(\w+)(,\s*([^)]*))?\)/.match(l)
      num = m[1].to_i
      name = m[2]

      if m[3]
        args = m[4].split(/\s*,\s*/)
      else
        args = []
      end

      syscalls[num] = Syscall.new(num, name, args)
    end
  end
end


File.open File.expand_path("../syscall_decl.incl.hpp", __FILE__), "w" do |f|
  syscalls.keys.sort.each do |num|
    sys = syscalls[num]

    n = sys.num_args

    next if sys.raw?

    if n == 0
      f.puts "DECL_SYSCALL0(#{sys.name});"
    else
      f.puts "DECL_SYSCALL#{n}(#{sys.name}, #{sys.decl_args});"
    end
  end
end

File.open File.expand_path("../syscall_impl.incl.hpp", __FILE__), "w" do |f|
  syscalls.keys.sort.each do |num|
    sys = syscalls[num]

    next if sys.raw?

    n = sys.num_args

    if n == 0
      f.puts "DEFN_SYSCALL0(#{sys.name}, #{sys.num});"
    else
      f.puts "DEFN_SYSCALL#{n}(#{sys.name}, #{sys.num}, #{sys.decl_args});"
    end
  end
end

File.open File.expand_path("../syscall_tramp.incl.hpp", __FILE__), "w" do |f|
  syscalls.keys.sort.each do |num|
    sys = syscalls[num]

    n = sys.num_args

    f.puts "void _syscall_tramp_#{sys.name}(Registers* regs) {"
    f.puts "  TRACE_START_SYSCALL(#{sys.num});"

    if sys.raw?
      f.puts "  SYSCALL_NAME(#{sys.name})(regs);"
    else
      f.puts "  regs->eax = SYSCALL_NAME(#{sys.name})(#{sys.arg_regs});"
    end

    f.puts "  TRACE_END_SYSCALL(#{sys.num});"
    f.puts "}"
  end

  f.puts "static void* syscalls[] = {"

  syscalls.keys.sort.each do |num|
    sys = syscalls[num]

    f.puts "  (void*)&_syscall_tramp_#{sys.name},"
  end

  f.puts "  0"
  f.puts "};"

  f.puts "const static u32 num_syscalls = #{syscalls.size};"

  f.puts "static const char* syscall_names[] = {"

  syscalls.keys.sort.each do |num|
    sys = syscalls[num]

    f.puts "  \"#{sys.name}\","
  end

  f.puts "  0"
  f.puts "};"

end
