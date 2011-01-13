module MilterMultiProcessTestUtils
  include MilterTestUtils

  private
  def setup_workers(client)
    if n_workers = ENV["MILTER_N_WORKERS"]
      n_workers = Integer(n_workers)
    end
    n_workers ||= 0
    @n_workers = n_workers
    if n_workers > 0
      client.n_workers = n_workers
    end
  end
end
