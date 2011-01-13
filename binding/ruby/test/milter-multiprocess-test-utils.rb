module MilterMultiprocessTestUtils
  include MilterTestUtils

  private
  def setup_workers(client)
    if n_workers = ENV["MILTER_N_WORKERS"] and (n_workers = Integer(n_workers)) > 0
      @n_workers = n_workers
      @client.n_workers = n_workers
    end
  end
end
