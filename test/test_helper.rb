require 'test/unit'
require_relative '../lib/binary_io'
require_relative '../lib/file_entry'

include Test::Unit::Assertions

# Utility methods for the tests.
module TestHelper
  module_function

  def write_and_read(arc, options = {})
    buffer = BinaryIO.new
    content1 = rand_binary_string(30_000)
    content2 = rand_binary_string(1)
    arc.files.push(FileEntry.new('test.png', ->() { content1 }))
    arc.files.push(FileEntry.new('dir/test.txt', ->() { content2 }))
    arc.files.push(FileEntry.new('empty', ->() { '' }))
    arc.write_internal(buffer, options)

    buffer.seek(0, IO::SEEK_SET)
    arc.read_internal(buffer)
    assert_equal(3, arc.files.length)
    assert_equal('test.png', arc.files[0].file_name)
    assert_equal('dir/test.txt', arc.files[1].file_name.gsub(/\\/, '/'))
    assert_equal('empty', arc.files[2].file_name)
    assert_equal(content1, arc.files[0].data.call)
    assert_equal(content2, arc.files[1].data.call)
    assert_equal('', arc.files[2].data.call)
  end

  def rand_string(length)
    (1..length).map { rand(2) == 0 ? '#' : '.' } * ''
  end

  def rand_binary_string(length)
    (1..length).map { rand(0xff).chr } * ''
  end
end