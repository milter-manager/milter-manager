module Milter
  class Client
    class MailTransactionShelf
      include Enumerable

      def initialize(context)
        @context = context
        @shelf = context.mail_transaction_shelf
      end

      def [](key)
        value = @context.get_mail_transaction_shelf_value(key)
        YAML.load(value)
      end

      def []=(key, value)
        @context.set_mail_transaction_shelf_value(key, value.to_yaml)
      end

      def each(&block)
        @shelf.each(&block)
      end
    end
  end
end
